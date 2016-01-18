#include <passes/LoopChecker.h>
#include "Compiler.h"
#include "passes/Circuiter.h"
#include "passes/ReturnChecker.h"
#include "passes/TypeChecker.h"
#include "Filesystem.h"

Compiler::Compiler(const std::string& filename, std::string out_build, std::string out_exec)
		: out_build(out_build), out_exec(out_exec) {
	initialize(filename);

	for (auto& file : files) {
		compile(*file);
	}

	std::string command("llvm-link -o \"eb-mass.ll\" -S ");
	for (auto& file : files) {
		command += file->out_filename + " ";
	}
	//std::cout << command << std::endl;
	exec(command.c_str());

	/*for (auto& file : files) {
		std::remove(file->out_filename.c_str());
	}*/

	std::string out_s = concat_paths(out_build, "out.s");
	command = "llc -o " + out_s + " \"eb-mass\".ll";
	//std::cout << command << std::endl;
	exec(command.c_str());

	command = "clang -o " + (out_exec.empty() ? "out" : out_exec) + " " + out_s + " shim.a";
	//std::cout << command << std::endl;
	exec(command.c_str());
}

void Compiler::compile(File& file) {
	if (file.state != File::READY) return;
	file.state = File::IN_PROGRESS;

	Parser parser;
	parser.construct(file.module, file.tokens->get_tokens());

	// perform short circuiting transformations
	// (replacing || and && with if's when there are side effects)
	Circuiter circuiter;
	circuiter.shorten(file.module);

	// transforms expression ifs into regular ifs
	// checks every returning function returns on all paths
	// creates implicit returns when possible & necessary
	ReturnChecker return_checker;
	return_checker.check(file.module);

	State state(file.module);

	// checks loops & if the breaks/continues are valid
	LoopChecker loop_checker;
	loop_checker.check(file.module, state);

	// resolve dependencies
	std.add_operators(file.module);
	resolve(file.module, state);

	// infers and checks all the types & finishes resolving functions
	TypeChecker type_checker(std);
	type_checker.check(file.module, state);

	Builder builder;
	builder.build(file.module, state, file.out_filename);
	create_obj_file(file);

	file.state = File::FINISHED;
}

void Compiler::initialize(const std::string& filename, bool force_recompile) {
	std::string name = get_file_stem(filename);
	for (int i = 0; i < name.size(); i++){
		if ((i == 0 && !is_valid_ident_beginning(name[i])) || (i > 0 && !is_valid_ident(name[i]))) {
			throw Except("'" + filename + "' (" + name + ") is not a proper identifier");
		}
	}

	File* file = new File();
	files.push_back(std::unique_ptr<File>(file));
	file->module.name.push_back(name);
	file->module.add_import(file->module.name, file->module);
	file_tree.add(file->module.name, *file);
	std::string out_filename;
	for (auto& str : file->module.name) {
		out_filename += str + "-";
	}
	out_filename += ".ll";
	file->out_filename = concat_paths(out_build, out_filename);

	if (!force_recompile && last_modified(filename) < last_modified(file->out_filename + ".o")) {
		load_obj_file(*file);
		file->state = File::FINISHED;
		return;
	}

	file->stream.open(filename);
	if (!file->stream.is_open()) throw Except("Could not find '" + filename + "'");
	file->tokens.reset(new Tokenizer(file->stream));
	auto& traits = file->tokens->get_traits();
	for (auto& trait : traits) {
		switch (trait.first) {
			case Trait::INCLUDE:
				file->includes.push_back(trait.second);
				initialize(trait.second);
				break;
			case Trait::OUT_EXEC:  out_exec  = trait.second; break;
			case Trait::OUT_BUILD: out_build = trait.second; break;
		}
	}
}

void Compiler::resolve(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::IMPORT: {
				Import& import_item = (Import&)item;
				std::vector<std::string> vec;
				for (auto token : import_item.target) {
					vec.push_back(token->str());
				}
				import(module, vec, import_item.token);
			} break;
			case Item::GLOBAL: {
				Global& global = (Global&)item;
				bool exists = module.declare(global);
				if (exists) throw Except("Global with this name already exists", global.token);
				global.unique_name = combine(module.name, ".") + "." + global.token.str();
			} break;
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				bool exists = module.declare(func);
				if (exists) throw Except("Duplicate function definition", func.token);
				std::stringstream ss;
				ss << combine(module.name, ".") << "." << func.token.str();
				ss << "." << func.param_names.size() << "." << func.index;
				func.unique_name = ss.str();
			} break;
			case Item::STRUCT: {
				Struct& strukt = (Struct&)item;
				module.declare(strukt);
				strukt.unique_name = combine(module.name, ".") + "." + strukt.token.str();
			} break;
			case Item::MODULE: break;
		}
	}
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::IMPORT: break;
			case Item::GLOBAL: {
				resolve(module, ((Global&)item).var.type);
			} break;
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				for (Type& type : func.param_types) {
					resolve(module, type);
				}
				for (Type& type : func.named_param_types) {
					resolve(module, type);
				}
				resolve(module, func.return_type);
				state.descend(func.block);
				state.set_func(func);
				for (size_t j = 0; j < func.param_names.size(); j++) {
					state.declare(func.param_names[j]->str(), func.param_types[j])->is_param = true;
				}
				for (auto pair : func.named_param_map) {
					state.declare(pair.first, func.named_param_types[pair.second])->is_param = true;
				}
				resolve(module, func.block, state);
				state.ascend();
			} break;
			case Item::STRUCT: {
				Struct& strukt = (Struct&)item;
				for (Type& type : strukt.member_types) {
					resolve(module, type);
				}
			} break;
			case Item::MODULE: {
				resolve(((SubModule&)item).module, state);
			} break;
		}
	}
}

void Compiler::resolve(Module& module, const Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				if (decl.type_token == nullptr) {
					state.declare(statement.token.str(), Type::Invalid);
				} else {
					state.declare(statement.token.str(), Type::parse(*decl.type_token));
					resolve(module, state.get_var(statement.token.str())->type);
				}
			} break;
			case Statement::ASSIGNMENT: {
				Assignment& assign = (Assignment&)statement;
				auto accesses = resolve(module, state, assign.token);
				for (auto access : accesses) {
					assign.accesses.push_back(std::unique_ptr<AccessTok>(access));
				}
			} break;
			default: break;
		}
		for (Block* inner_block : statement.blocks()) {
			state.descend(*inner_block);
			resolve(module, *inner_block, state);
			state.ascend();
		}
		resolve(module, &statement.expr, state);
	}
}
void Compiler::resolve(Module& module, Expr* expr, State& state) {
	std::vector<std::pair<Tok*, Tok*>> insertions;
	for (size_t i = 0; i < expr->size(); i++) {
		Tok& tok = *(*expr)[i];
		if (!(tok.form == Tok::FUNC || tok.form == Tok::VAR)) continue;
		auto accesses = resolve(module, state, *tok.token, &tok);
		for (Tok* access : accesses) {
			insertions.emplace_back(&tok, access);
		}
	}
	insert(expr, insertions);
}

std::vector<AccessTok*> Compiler::resolve(Module& module, State& state, const Token& token,
                                          Tok* tok) {
	std::vector<AccessTok*> accesses;
	bool on_module = true;
	Module* cur_module = &module;
	auto& ident = token.ident();
	for (size_t j = 0; j < ident.size(); j++) {
		if (j == ident.size() - 1 && tok != nullptr && tok->form == Tok::FUNC) {
			FuncTok* ftok = (FuncTok*)tok;
			if (!on_module) throw Except("cant do func on struct yet...");
			auto& funcs = cur_module->get_functions(ftok->num_unnamed_args, ident[j]);
			std::vector<Function*> possible_funcs;
			for (auto func : funcs) {
				if (func->pub || j == 0) possible_funcs.push_back(func);
			}
			ftok->possible_funcs = possible_funcs;
			if (cur_module != &module) ftok->external = true;
			break;
		} else if (j == 0) {
			Variable* var = state.get_var(ident[j]);
			if (var != nullptr) {
				on_module = false;
				if (tok != nullptr) dynamic_cast<VarTok*>(tok)->var = var;
				continue;
			}
		}
		if (on_module) {
			Global* glob = cur_module->get_global(ident[j]);
			if (glob == nullptr) {
				std::vector<std::string> vec(1, ident[j]);
				cur_module = cur_module->search(vec);
				if (cur_module == nullptr) {
					cur_module = &import(state.get_module(), vec, token);
				}
			} else {
				if (!glob->pub) throw Except("Can't access private global", token);
				on_module = false;
			}
		} else {
			accesses.push_back(new AccessTok(token, ident[j]));
		}
	}
	return accesses;
}

void Compiler::resolve(Module& module, Type& type) {
	if (type == Type::Unresolved) {
		assert(type.token != nullptr);
		if (type.token->ident().size() > 1) {
			// if identifier length not 1, it's assumed to not be in this module
			auto vec = type.token->ident();
			vec.pop_back();
			Module* mod = module.search(vec);
			if (mod == nullptr) {
				mod = &import(module, vec, *type.token);
			}
			Struct* strukt = mod->get_struct(type.token->ident().back());
			if (strukt == nullptr) throw Except("Couldn't resolve type", *type.token);
			if (!strukt->pub) throw Except("Can't access private struct", *type.token);
			type.form = Type::STRUCT;
			type.strukt = strukt;
			module.external_items.insert(strukt);
		} else {
			Struct* strukt = module.get_struct(type.token->str());
			if (strukt == nullptr) throw Except("Couldn't resolve type", *type.token);
			type.form = Type::STRUCT;
			type.strukt = strukt;
		}
	}
}

Module& Compiler::import(Module& module, const std::vector<std::string>& name, const Token& token) {
	File* file = file_tree.search(name);
	if (file == nullptr) {
		throw Except("Module could not be found", token);
	}
	if (file->state == File::READY) {
		compile(*file);
	} else if (file->state == File::IN_PROGRESS) {
		throw Except("Circular dependency!", token);
	}
	module.add_import(name, file->module);
	return file->module;
}

// Obj:
// [bitcode length, 8], {module bitcode}
// [num includes]{filename...}
// [num functions, 4]{(name, return type, [num params, 1], {param type...})...}
// [num globals, 4]{(const, name, type)...}
// Type:
// [primitive, 1]
// S, T, E: (structure, tuple, enum)
// *, &, +, ^: references
void write_type(std::ofstream& out, Type type) {
	std::unordered_map<Type, char> types = {
			{Type::Void, 'v'}, {Type::Bool, 'b'}, {Type::IPtr, 'p'}, {Type::UPtr, 'u'},
			{Type::I8, 'c'}, {Type::I16, 's'}, {Type::I32, 'i'}, {Type::I32, 'l'},
			{Type::U8, '1'}, {Type::U16, '2'}, {Type::U32, '4'}, {Type::U64, '8'},
			{Type::Int, 'I'}, {Type::F32, 'f'}, {Type::F64, 'd'}, {Type::Float, 'F'}
	};
	auto iter = types.find(type);
	if (iter == types.end()) {
		static char invalid = '!';
		out.write(&invalid, 1);
		return;
	}
	out.write(&iter->second, 1);
}
Type read_type(std::ifstream& in) {
	std::unordered_map<char, Type> types = {
			{'v', Type::Void}, {'b', Type::Bool}, {'p', Type::IPtr}, {'u', Type::UPtr},
			{'c', Type::I8}, {'s', Type::I16}, {'i', Type::I32}, {'l', Type::I32},
			{'1', Type::U8}, {'2', Type::U16}, {'4', Type::U32}, {'8', Type::U64},
			{'I', Type::Int}, {'f', Type::F32}, {'d', Type::F64}, {'F', Type::Float}
	};
	char c;
	in.read(&c, 1);
	auto iter = types.find(c);
	assert(iter != types.end());
	return iter->second;
}
void write_value(std::ofstream& out, Value value) {
	static char fc = 'f', ic = 'i', bc = 'b';
	if (value.type.is_float()) {
		out.write(&fc, 1);
		uint64_t i = value.i();
		out.write((char*)&i, sizeof(uint64_t));
	} else if (value.type.is_int()) {
		out.write(&ic, 1);
		double f = value.f();
		out.write((char*)&f, sizeof(double));
	} else if (value.type == Type::Bool) {
		out.write(&bc, 1);
		char b = value.b();
		out.write(&b, 1);
	}
}
Value read_value(std::ifstream& in, Type type) {
	char c;
	in.read(&c, 1);
	if (c == 'f') {
		double f;
		in.read((char*)&f, sizeof(double));
		return Value(f, type);
	} else if (c == 'i') {
		uint64_t i;
		in.read((char*)&i, sizeof(uint64_t));
		return Value(i, type);
	} else if (c == 'b') {
		char b;
		in.read(&b, 1);
		return Value((bool)b);
	}
	assert(false);
	return Value();
}
void Compiler::create_obj_file(File& file) {
	std::ifstream in(file.out_filename, std::ifstream::ate | std::ifstream::binary);
	int64_t len = (int64_t)in.tellg();
	assert(in.good());
	in.seekg(0, std::ios::beg);

	std::string obj_filename = file.out_filename + ".o";
	std::ofstream out(obj_filename, std::ifstream::binary);
	out << 'e' << 'b' << '$';
	out.write((char*)&len, 8);
	out << in.rdbuf();

	int32_t num_includes = (int32_t)file.includes.size();
	out.write((char*)&num_includes, 4);
	for (auto& filename : file.includes) {
		out << filename;
	}

	int32_t num_funcs = (int32_t)file.module.get_pub_functions().size();
	out.write((char*)&num_funcs, 4);
	for (Function* func : file.module.get_pub_functions()) {
		out << func->token.str();
		write_type(out, func->return_type);
		uint8_t num_params = (uint8_t)func->param_types.size();
		out.write((char*)&num_params, 1);
		for (Type& type : func->param_types) {
			write_type(out, type);
		}
	}

	int32_t num_globals = (int32_t)file.module.get_pub_globals().size();
	out.write((char*)&num_globals, 4);
	for (Global* global : file.module.get_pub_globals()) {
		char conzt = global->conzt;
		out.write(&conzt, 1);
		out << global->token.str();
		write_type(out, global->var.type);
		write_value(out, global->val);
	}
}
void Compiler::load_obj_file(File& file) {
	std::string obj_filename = file.out_filename + ".o";
	std::ifstream in(obj_filename, std::ifstream::binary);
	char eb[3];
	in.read(eb, 3);
	if (std::string(eb, 3) != "eb$") throw Except("Invalid obj file '" + obj_filename + "'");
	int64_t len;
	in.read((char*)&len, 8);

	char mem[len]; // TODO: reuse buffer
	in.read(mem, len);
	std::ofstream out(file.out_filename);
	out.write(mem, len);

	int32_t num_includes;
	in.read((char*)&num_includes, 4);
	for (size_t i = 0; i < num_includes; i++) {
		std::string filename;
		in >> filename;
		initialize(filename, true);
	}

	int32_t num_funcs;
	in.read((char*)&num_funcs, 4);
	for (size_t i = 0; i < num_funcs; i++) {
		std::string func_name;
		in >> func_name;
		extra_tokens.push_back(Token(Token::IDENT, func_name));
		Function* func = new Function(extra_tokens.back());
		func->return_type = read_type(in);
		uint8_t num_params;
		in.read((char*)&num_params, 1);
		for (size_t j = 0; j < num_params; j++) {
			func->param_types.push_back(read_type(in));
			std::string param_name = "eb$p" + j;
			extra_tokens.push_back(Token(Token::IDENT, param_name));
			func->param_names.push_back(&extra_tokens.back());
		}
		extra_functions.push_back(std::unique_ptr<Function>(func));
	}

	int32_t num_globals;
	in.read((char*)&num_globals, 4);
	for (size_t i = 0; i < num_globals; i++) {
		char conzt;
		in.read(&conzt, 1);
		std::string global_name;
		in >> global_name;
		extra_tokens.push_back(Token(Token::IDENT, global_name));
		Type type = read_type(in);
		Value val = read_value(in, type);
		Global* global = new Global(extra_tokens.back(), type, val, conzt);
		extra_globals.push_back(std::unique_ptr<Global>(global));
	}
}
