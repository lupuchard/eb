#include "passes/TypeChecker.h"
#include "Except.h"
#include <algorithm>

TypeChecker::TypeChecker(Std& std): std(std) { }

void TypeChecker::check(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				state.set_func(func);
				check(module, func.block, state);
			} break;
			default: break;
		}
	}
}

void TypeChecker::check(Module& mod, Block& block, State& state) {
	state.descend(block);
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				Variable& var = *state.get_var(decl.token.str());
				if (!decl.expr.empty()) {
					var.type = check(mod, &decl.expr, state, decl.token, var.type);
				}
			} break;
			case Statement::ASSIGNMENT: {
				Assignment& assign = dynamic_cast<Assignment&>(statement);
				Variable* var = state.get_var(assign.token.str());
				if (var == nullptr) {
					throw Except("No variable of this name found", assign.token);
				} else if (var->is_param) {
					throw Except("You may not assign to parameters", assign.token);
				} else if (var->is_const) {
					throw Except("You may not assign to const globals", assign.token);
				}
				Type type = var->type;
				for (size_t j = 0; j < assign.accesses.size(); j++) {
					AccessTok& access = *assign.accesses[j];
					if (!type.is_struct()) throw Except("Can only access structs", assign.token);
					access.idx = type.strukt->member_map[access.name];
					type = type.strukt->member_types[access.idx];
				}
				type = check(mod, &statement.expr, state, statement.token, type);
				if (assign.accesses.empty()) var->type = type;
			} break;
			case Statement::EXPR: {
				check(mod, &statement.expr, state, statement.token);
			} break;
			case Statement::RETURN: {
				if (state.get_func().return_type == Type::Void) break;
				check(mod, &statement.expr, state, statement.token, state.get_func().return_type);
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				check(mod, &statement.expr, state, statement.token, Type::Bool);
				check(mod, if_statement.true_block, state);
				check(mod, if_statement.else_block, state);
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				check(mod, &statement.expr, state, statement.token, Type::Bool);
				check(mod, while_statement.block, state);
			} break;
			default: break;
		}
	}
	state.ascend();
}

Type TypeChecker::check(Module& mod, Expr* expr, State& state, const Token& token, Type res) {
	// implicit casts to be inserted
	std::map<Tok*, Tok*> insertions;

	// evaluation stack
	std::vector<Type> stack;
	std::vector<Tok*> tok_stack;

	for (size_t j = 0; j < expr->size(); j++) {
		Tok& tok = *(*expr)[j];
		switch (tok.form) {
			case Tok::VAR:   stack.push_back(((VarTok&)tok).var->type);    break;
			case Tok::VALUE: stack.push_back(((ValueTok&)tok).value.type); break;
			case Tok::ACCESS: {
				AccessTok& atok = (AccessTok&)tok;
				if (!stack.back().is_struct()) {
					throw Except("Cannot access on non-structure type", *tok.token);
				}
				Struct& strukt = *stack.back().strukt;
				auto it = strukt.member_map.find(atok.name);
				if (it == strukt.member_map.end()) throw Except("Member not found", *tok.token);
				atok.idx = it->second;
				stack.back() = strukt.member_types[atok.idx];
				tok_stack.pop_back();
			} break;
			case Tok::FUNC: {
				FuncTok& ftok = (FuncTok&)tok;
				if (ftok.possible_funcs.empty()) throw Except("Function not found", *tok.token);

				std::vector<Type> args(    stack.end() - ftok.num_args,     stack.end());
				std::vector<Tok*> toks(tok_stack.end() - ftok.num_args, tok_stack.end());
				stack.erase(        stack.end() - ftok.num_args,     stack.end());
				tok_stack.erase(tok_stack.end() - ftok.num_args, tok_stack.end());

				// function overloading means there are multiple choices
				// this chooses the function that requires the fewest implicit casts to reach
				int min_num_casts = 255;
				std::vector<Function*> valid_funcs;
				for (Function* func : ftok.possible_funcs) {
					bool match = true;
					int num_casts = 0;
					for (size_t i = 0; i < ftok.num_unnamed_args; i++) {
						Type& param = func->param_types[i];
						if (param == args[i]) continue;
						if (std.get_cast(args[i], param) != nullptr) {
							num_casts++;
							if (num_casts > min_num_casts) {
								match = false;
								break;
							}
							continue;
						}
						match = false;
						break;
					}
					if (match) {
						if (num_casts < min_num_casts) {
							valid_funcs.clear();
							min_num_casts = num_casts;
						}
						valid_funcs.push_back(func);
					}
				}

				if (valid_funcs.empty()) {
					throw Except("Arguments match no function", *tok.token);
				} else if (valid_funcs.size() > 1) {
					if (valid_funcs[0]->form == Function::OP && valid_funcs.size() == 2) {
						// if an operator call is ambiguous, it chooses the one for the larger type
						Type type0 = valid_funcs[0]->param_types[0];
						Type type1 = valid_funcs[1]->param_types[0];
						if (type0 == Type::IntLit || type0.size() < type1.size()) {
							valid_funcs[0] = valid_funcs[1];
						}
						valid_funcs.pop_back();
					} else {
						throw Except("Function call is ambiguous", *tok.token);
					}
				}

				Function& func = *valid_funcs[0];
				for (size_t i = 0; i < ftok.num_unnamed_args; i++) {
					insert_cast(token, insertions, toks[i], args[i], func.param_types[i]);
				}
				for (int i = ftok.num_unnamed_args; i < ftok.num_args; i++) {
					std::string arg_name = ftok.named_args[i - ftok.num_unnamed_args];
					Type type = func.named_param_types[func.named_param_map[arg_name]];
					insert_cast(token, insertions, toks[i], args[i], type);
				}

				ftok.possible_funcs = valid_funcs;
				if (ftok.external) mod.external_items.insert(&func);
				stack.push_back(func.return_type);
			} break;
			case Tok::IF: assert(false);
		}
		tok_stack.push_back(&tok);
	}
	insert(expr, insertions);

	if (res == Type::Invalid || res == stack.back()) return stack.back();
	Function* cast = std.get_cast(stack.back(), res);
	if (cast == nullptr) {
		throw Except(stack.back().to_string() + " does not match " + res.to_string(), token);
	}
	FuncTok* ftok = new FuncTok(token, 1);
	ftok->possible_funcs.push_back(cast);
	expr->push_back(std::unique_ptr<Tok>(ftok));
	return res;
}

void TypeChecker::insert_cast(const Token& token, std::map<Tok*, Tok*>& insertions, Tok* tok,
                              Type arg, Type param) {
	if (param != arg) {
		FuncTok* new_ftok = new FuncTok(token, 1);
		Function* cast = std.get_cast(arg, param);
		if (cast == nullptr) {
			throw Except("Cannot cast from " + arg.to_string() + " to " + param.to_string(), token);
		}
		new_ftok->possible_funcs.push_back(cast);
		insertions[tok] = new_ftok;
	}
}
