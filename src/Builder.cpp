#include "Builder.h"
#include "util/Filesystem.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include <fstream>

void Builder::build(Module& module, State& state, const std::string& out_file) {
	llvm::Module llvm_module("thang_main", llvm::getGlobalContext());
	c = &llvm_module.getContext();

	for (auto func : module.external_functions) {
		llvm::Type* result = type_to_llvm(func->return_type);
		std::vector<llvm::Type*> args;
		for (size_t i = 0; i < func->param_names.size(); i++) {
			args.push_back(type_to_llvm(func->param_types[i]));
		}
		auto llvm_args = llvm::ArrayRef<llvm::Type*>(args);
		llvm::FunctionType* llvm_func = llvm::FunctionType::get(result, llvm_args, false);
		llvm_functions[func] = llvm_module.getOrInsertFunction(func->unique_name, llvm_func);
	}
	for (auto global : module.external_globals) {
		llvm::Type* llvm_type = type_to_llvm(global->var.type);
		global->var.llvm = llvm_module.getOrInsertGlobal(global->unique_name, llvm_type);
	}

	do_module(module, llvm_module, state);

	std::ofstream file;
	create_directory(out_file);
	file.open(out_file);
	llvm::raw_os_ostream stream(file);

	if (out_file.back() == 'c') {
		// bitcode?
		llvm::WriteBitcodeToFile(&llvm_module, stream);
	} else {
		llvm::PassManager pass_manager;
		pass_manager.add(llvm::createPrintModulePass(&stream));
		pass_manager.run(llvm_module);
	}
}

llvm::Type* Builder::type_to_llvm(Type& type) {
	if (type == Type::Float) {
		switch (sizeof(long double)) {
			case 8:  return llvm::Type::getDoubleTy(*c);
			case 10: return llvm::Type::getX86_FP80Ty(*c);
			case 16: return llvm::Type::getFP128Ty(*c);
			default: assert(false); break;
		}
	}
	static std::unordered_map<Type, llvm::Type*> types = {
			{Type::Void, llvm::Type::getVoidTy(*c)}, {Type::Bool, llvm::Type::getInt1Ty(*c)},
			{Type::IntLit, llvm::IntegerType::get(*c, 8 * sizeof(uintmax_t))},
			{Type::Int,    llvm::IntegerType::get(*c, 8 * sizeof(int_fast32_t))},
			{Type::IPtr,   llvm::IntegerType::get(*c, 8 * sizeof(intptr_t))},
			{Type::UPtr,   llvm::IntegerType::get(*c, 8 * sizeof(uintptr_t))},
			{Type::I8,  llvm::Type::getInt8Ty(*c)},  {Type::U8,  llvm::Type::getInt8Ty(*c)},
			{Type::I16, llvm::Type::getInt16Ty(*c)}, {Type::U16, llvm::Type::getInt16Ty(*c)},
			{Type::I32, llvm::Type::getInt32Ty(*c)}, {Type::U32, llvm::Type::getInt32Ty(*c)},
			{Type::I64, llvm::Type::getInt64Ty(*c)}, {Type::U64, llvm::Type::getInt64Ty(*c)},
			{Type::F32, llvm::Type::getFloatTy(*c)}, {Type::F64, llvm::Type::getDoubleTy(*c)}
	};
	auto iter = types.find(type);
	assert(iter != types.end());
	return iter->second;
}

void Builder::do_module(Module& module, llvm::Module& llvm_module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;

				llvm::Type* ret = type_to_llvm(func.return_type);
				std::vector<llvm::Type*> params;
				for (size_t j = 0; j < func.param_types.size(); j++) {
					params.push_back(type_to_llvm(func.param_types[j]));
				}
				llvm_func = llvm::cast<llvm::Function>(llvm_module.getOrInsertFunction(
						func.token.str() == "main" ? "eb$main" : func.unique_name,
						llvm::FunctionType::get(ret, llvm::ArrayRef<llvm::Type*>(params), false)
				));
				llvm_functions[&func] = llvm_func;

				state.descend(func.block);
				auto iter = llvm_func->arg_begin();
				for (size_t j = 0; j < func.param_types.size(); j++) {
					state.get_var(func.param_names[j]->str())->llvm = &*iter++;
				}
				llvm::IRBuilder<> builder(create_basic_block("entry"));

				do_block(builder, func.block, state);

				state.ascend();
			} break;
			case Item::GLOBAL: {
				Global& global = (Global&)item;
				llvm::Type* type = type_to_llvm(global.var.type);
				llvm::GlobalVariable* llvm_global = llvm::cast<llvm::GlobalVariable>(
						llvm_module.getOrInsertGlobal(global.unique_name, type)
				);
				llvm_global->setInitializer(default_value(global.var.type, type));
				global.var.llvm = llvm_global;
			} break;
			case Item::IMPORT: break;
		}
	}
}

// returns whether or not the block ended on a return
bool Builder::do_block(llvm::IRBuilder<>& builder, Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		do_statement(builder, statement, state);
		if (statement.form == Statement::RETURN ||
			statement.form == Statement::BREAK ||
			statement.form == Statement::CONTINUE) {
			return true;
		}
	}
	return false;
}

llvm::Value* Builder::do_statement(llvm::IRBuilder<>& b, Statement& statement, State& state) {
	llvm::Value* drop = nullptr;
	switch (statement.form) {
		case Statement::DECLARATION: {
			Declaration& decl = (Declaration&)statement;
			Variable& var = *state.get_var(decl.token.str());
			auto llvm_type = type_to_llvm(var.type);
			auto llvm_val = b.CreateAlloca(llvm_type, nullptr, decl.token.str());
			var.llvm = llvm_val;
			llvm::Value* assigned;
			if (decl.expr.empty()) {
				// TODO: require all variables be initialized before used, so no defaults
				assigned = default_value(var.type, llvm_type);
			} else {
				assigned = do_expr(b, decl.expr, state);
			}
			if (assigned != nullptr) {
				b.CreateStore(assigned, llvm_val);
			}
		} break;
		case Statement::ASSIGNMENT: {
			llvm::Value* assigned = do_expr(b, statement.expr, state);
			b.CreateStore(assigned, state.get_var(statement.token.str())->llvm);
		} break;
		case Statement::EXPR: {
			drop = do_expr(b, statement.expr, state);
		} break;
		case Statement::RETURN: {
			llvm::Value* returned = do_expr(b, statement.expr, state);
			b.CreateRet(returned);
		} break;
		case Statement::IF: {
			If& if_statement = (If&)statement;
			bool exits = false;
			llvm::Value* cond = do_expr(b, if_statement.expr, state);
			llvm::BasicBlock* if_true  = create_basic_block("if");
			llvm::BasicBlock* if_false = create_basic_block("else");
			llvm::BasicBlock* end      = create_basic_block("end");
			b.CreateCondBr(cond, if_true, if_false);
			b.SetInsertPoint(if_true);
			state.descend(if_statement.true_block);
			if (!do_block(b, if_statement.true_block, state)) {
				b.CreateBr(end);
				exits = true;
			}
			state.ascend();
			b.SetInsertPoint(if_false);
			state.descend(if_statement.else_block);
			if (!do_block(b, if_statement.else_block, state)) {
				b.CreateBr(end);
				exits = true;
			}
			state.ascend();
			if (exits) b.SetInsertPoint(end);
			else end->eraseFromParent();
		} break;
		case Statement::WHILE: {
			While& while_statement = (While&)statement;
			llvm::BasicBlock* start   = create_basic_block("start");
			llvm::BasicBlock* if_true = create_basic_block("loop");
			llvm::BasicBlock* end     = create_basic_block("end");
			b.CreateBr(start);
			b.SetInsertPoint(start);
			llvm::Value* cond = do_expr(b, while_statement.expr, state);
			b.CreateCondBr(cond, if_true, end);
			b.SetInsertPoint(if_true);
			state.descend(while_statement.block);
			Loop& loop = *state.get_loop(1);
			loop.start = start;
			loop.end   = end;
			do_block(b, while_statement.block, state);
			state.ascend();
			b.CreateBr(start);
			b.SetInsertPoint(end);
		} break;
		case Statement::CONTINUE:
			b.CreateBr(state.get_loop(1)->start);
			break;
		case Statement::BREAK: {
			Break& break_statement = (Break&)statement;
			b.CreateBr(state.get_loop(break_statement.amount)->end);
		} break;
	}
	return drop;
}

llvm::Value* Builder::do_expr(llvm::IRBuilder<>& builder, Expr& expr, State& state) {
	std::vector<llvm::Value*> value_stack;
	std::vector<Type*> type_stack;
	for (size_t j = 0; j < expr.size(); j++) {
		Tok& tok = *expr[j];
		switch (tok.form) {
			case Tok::INT: {
				auto i = ((IntTok&)tok).i;
				if (tok.type.is_float()) {
					value_stack.push_back(llvm::ConstantFP::get( type_to_llvm(tok.type), i));
				} else {
					value_stack.push_back(llvm::ConstantInt::get(type_to_llvm(tok.type), i));
				}
				type_stack.push_back(&tok.type);
			} break;
			case Tok::FLOAT: {
				double f = ((FloatTok&)tok).f;
				value_stack.push_back(llvm::ConstantFP::get(type_to_llvm(tok.type), f));
				type_stack.push_back(&tok.type);
			} break;
			case Tok::VAR: {
				Variable& var = *state.get_var(tok.token->str());
				if (var.is_param) {
					value_stack.push_back(var.llvm);
				} else {
					value_stack.push_back(builder.CreateLoad(var.llvm, tok.token->str().c_str()));
				}
				type_stack.push_back(&var.type);
			} break;
			case Tok::FUNC: {
				FuncTok& ftok = (FuncTok&)tok;
				assert(ftok.possible_funcs.size() == 1);
				Function& func = *ftok.possible_funcs[0];
				std::vector<llvm::Value*> args;
				args.resize(func.param_types.size());
				for (int i = (int)func.param_types.size() - 1; i >= 0; i--) {
					args[i] = value_stack.back();
					value_stack.pop_back();
					type_stack.pop_back();
				}
				if (func.form == Function::OP) {
					value_stack.push_back(do_op(builder, func, args));
				} else if (func.form == Function::CAST) {
					value_stack.push_back(do_cast(builder, func, args[0]));
				} else {
					assert(llvm_functions.count(ftok.possible_funcs[0]));
					value_stack.push_back(builder.CreateCall(
							llvm_functions[&func],
							llvm::ArrayRef<llvm::Value*>(args), func.token.str()
					));
				}
				type_stack.push_back(&func.return_type);
			} break;
			default: assert(false);
		}
	}
	return value_stack.back();
}

llvm::Constant* Builder::default_value(Type& type, llvm::Type* llvm_type) {
	if (type.is_float()) {
		return llvm::ConstantFP::get(llvm_type, 0);
	} else if (type.is_int()) {
		return llvm::ConstantInt::get(llvm_type, 0);
	} else {
		return nullptr;
	}
}

llvm::Value* Builder::do_op(llvm::IRBuilder<>& builder, Function& op,
                            std::vector<llvm::Value*>& args) {
	if (args.size() == 2) {
		auto a = args[0];
		auto b = args[1];
		Type type = op.param_types[0];
		switch (op.token.str()[0]) {
			case '+': return type.is_float()  ? builder.CreateFAdd(a, b) :
			                 type.is_signed() ? builder.CreateNSWAdd(a, b) :
			                                    builder.CreateNUWAdd(a, b);
			case '-': return type.is_float()  ? builder.CreateFSub(a, b) :
			                 type.is_signed() ? builder.CreateNSWSub(a, b) :
			                                    builder.CreateNUWSub(a, b);
			case '*': return type.is_float()  ? builder.CreateFMul(a, b) :
			                 type.is_signed() ? builder.CreateNSWMul(a, b) :
			                                    builder.CreateNUWMul(a, b);
			case '/': return type.is_float()  ? builder.CreateFDiv(a, b) :
			                 type.is_signed() ? builder.CreateSDiv(a, b) :
			                                    builder.CreateUDiv(a, b);
			case '%': return type.is_float()  ? builder.CreateFRem(a, b) :
			                 type.is_signed() ? builder.CreateSRem(a, b) :
			                                    builder.CreateURem(a, b);
			case '&': return builder.CreateAnd(a, b);
			case '|': return builder.CreateOr( a, b);
			case '^': return builder.CreateXor(a, b);
			case '=': return type.is_float()  ? builder.CreateFCmpUEQ(a, b) :
			                                    builder.CreateICmpEQ(a, b);
			case '!': return type.is_float()  ? builder.CreateFCmpUNE(a, b) :
			                                    builder.CreateICmpNE(a, b);
			case '>': return op.token.str().size() == 1 ? (
						type.is_float()  ? builder.CreateFCmpUGT(a, b) :
						type.is_signed() ? builder.CreateICmpSGT(a, b) :
						                   builder.CreateICmpUGT(a, b)
				) : op.token.str()[1] == '=' ? (
						type.is_float()  ? builder.CreateFCmpUGE(a, b) :
						type.is_signed() ? builder.CreateICmpSGE(a, b) :
						                   builder.CreateICmpUGE(a, b)
				) : type.is_signed() ? builder.CreateAShr(a, b) : builder.CreateLShr(a, b);
			case '<': return op.token.str().size() == 1 ? (
						type.is_float()  ? builder.CreateFCmpULT(a, b) :
						type.is_signed() ? builder.CreateICmpSLT(a, b) :
						                   builder.CreateICmpULT(a, b)
				) : op.token.str()[1] == '=' ? (
						type.is_float()  ? builder.CreateFCmpULE(a, b) :
						type.is_signed() ? builder.CreateICmpSLE(a, b) :
						                   builder.CreateICmpULE(a, b)
				) : builder.CreateShl(a, b);
			default: assert(false); return nullptr;
		}
	} else {
		auto a = args[0];
		Type type = op.param_types[0];
		switch (op.token.str()[0]) {
			case '!': return builder.CreateNot(a);
			case '-': return type.is_float()  ? builder.CreateFNeg(a) :
			                 type.is_signed() ? builder.CreateNSWNeg(a) :
		                                        builder.CreateNUWNeg(a);
			case '/': return type.is_float()  ? builder.CreateFDiv(llvm::ConstantFP::get(a->getType(), 0), a) :
			                 type.is_signed() ? builder.CreateSDiv(llvm::ConstantInt::get(a->getType(), 0), a) :
		                                        builder.CreateUDiv(llvm::ConstantInt::get(a->getType(), 0), a);
			default: assert(false); return nullptr;
		}
	}
}

llvm::Value* Builder::do_cast(llvm::IRBuilder<>& builder, Function& cast, llvm::Value* arg) {
	if (cast.return_type.is_float()) {
		return builder.CreateSIToFP(arg, type_to_llvm(cast.return_type));
	} else if (cast.param_types[0].is_signed()) {
		return builder.CreateSExtOrTrunc(arg, type_to_llvm(cast.return_type));
	} else {
		return builder.CreateZExtOrTrunc(arg, type_to_llvm(cast.return_type));
	}
}

llvm::BasicBlock* Builder::create_basic_block(std::string name) {
	return llvm::BasicBlock::Create(*c, name, llvm_func);
}
