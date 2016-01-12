#include "passes/TypeChecker.h"
#include "Exception.h"

TypeChecker::TypeChecker(Std& std): std(std) { }

void TypeChecker::check(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				state.set_func(func);
				check(func.block, state);
			} break;
			default: break;
		}
	}
}

void TypeChecker::check(Block& block, State& state) {
	state.descend(block);
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		const Token& token = statement.token;
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				Variable& var = *state.get_var(decl.token.str());
				if (!decl.expr.empty()) {
					var.type = check(&decl.expr, state, decl.token, var.type);
				}
			} break;
			case Statement::ASSIGNMENT: {
				Variable* var = state.get_var(statement.token.str());
				if (var == nullptr) {
					throw Exception("No variable of this name found", statement.token);
				} else if (var->is_param) {
					throw Exception("You may not assign to parameters", statement.token);
				} else if (var->is_const) {
					throw Exception("You may not assign to const globals", statement.token);
				}
				var->type = check(&statement.expr, state, statement.token, var->type);
			} break;
			case Statement::EXPR: {
				check(&statement.expr, state, statement.token);
			} break;
			case Statement::RETURN: {
				if (state.get_func().return_type == Type::Void) break;
				check(&statement.expr, state, statement.token, state.get_func().return_type);
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				check(&statement.expr, state, statement.token, Type::Bool);
				check(if_statement.true_block, state);
				check(if_statement.else_block, state);
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				check(&statement.expr, state, statement.token, Type::Bool);
				check(while_statement.block, state);
			} break;
			default: break;
		}
	}
	state.ascend();
}

Type TypeChecker::check(Expr* expr, State& state, const Token& token, Type res) {
	std::map<Tok*, Tok*> insertions;
	std::vector<Type> stack;
	std::vector<Tok*> tok_stack;
	for (size_t j = 0; j < expr->size(); j++) {
		Tok& tok = *(*expr)[j];
		if (tok.form == Tok::VAR) {
			stack.push_back(((VarTok&)tok).var->type);
		} else if (tok.form == Tok::FUNC) {
			FuncTok& ftok = (FuncTok&)tok;
			if (ftok.possible_funcs.empty()) throw Exception("Function not found", *tok.token);

			std::vector<Type> args(    stack.end() - ftok.num_params,     stack.end());
			std::vector<Tok*> toks(tok_stack.end() - ftok.num_params, tok_stack.end());
			stack.erase(        stack.end() - ftok.num_params,     stack.end());
			tok_stack.erase(tok_stack.end() - ftok.num_params, tok_stack.end());

			int min_num_casts = 255;
			std::vector<Function*> valid_funcs;
			for (Function* func : ftok.possible_funcs) {
				bool match = true;
				int num_casts = 0;
				for (size_t i = 0; i < ftok.num_params; i++) {
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
				throw Exception("Arguments match no function", *tok.token);
			} else if (valid_funcs.size() > 1) {
				throw Exception("Function call is ambiguous", *tok.token);
			}

			for (size_t i = 0; i < ftok.num_params; i++) {
				Type& param = valid_funcs[0]->param_types[i];
				if (valid_funcs[0]->param_types[i] != args[i]) {
					FuncTok* new_ftok = new FuncTok(token, 1);
					assert(std.get_cast(args[i], param) != nullptr);
					new_ftok->possible_funcs.push_back(std.get_cast(args[i], param));
					insertions[toks[i]] = new_ftok;
				}
			}
			ftok.possible_funcs = valid_funcs;
			stack.push_back(valid_funcs[0]->return_type);
		} else {
			stack.push_back(tok.type);
		}
		tok_stack.push_back(&tok);
	}

	if (!insertions.empty()) {
		Expr new_expr;
		for (auto& tok : *expr) {
			auto iter = insertions.find(&*tok);
			new_expr.emplace_back();
			new_expr.back().swap(tok);
			if (iter != insertions.end()) {
				new_expr.push_back(std::unique_ptr<Tok>(iter->second));
			}
		}
		*expr = std::move(new_expr);
	}

	if (res == Type::Invalid || res == stack.back()) return stack.back();
	Function* cast = std.get_cast(stack.back(), res);
	if (cast == nullptr) {
		throw Exception(stack.back().to_string() + " does not match " + res.to_string(), token);
	}
	FuncTok* ftok = new FuncTok(token, 1);
	ftok->possible_funcs.push_back(cast);
	expr->push_back(std::unique_ptr<Tok>(ftok));
	return res;
}
