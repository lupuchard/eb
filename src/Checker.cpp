#include "Checker.h"
#include <cassert>
#include <iostream>

void Checker::check_types(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				bool exists = state.declare(func.name_token.str, func);
				if (exists) throw Exception("Duplicate function definition", func.name_token);
				state.descend(func.block);
				state.set_return(func.return_type);
				for (size_t j = 0; j < func.param_names.size(); j++) {
					state.declare(func.param_names[j]->str, func.param_types[j]).is_param = true;
				}
				check_types(func.block, state);
				state.ascend();
			} break;
		}
	}
}

void Checker::check_types(Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				Type type;
				if (decl.type_token != nullptr) {
					type = Type::parse(decl.type_token->str);
				}
				if (decl.expr.get() != nullptr) {
					type = type.merge(type_of(*decl.expr, state, decl.token));
				}
				state.declare(decl.token.str, type);
			} break;
			case Statement::ASSIGNMENT: {
				Type type = type_of(*statement.expr, state, statement.token);
				Variable* var = state.get_var(statement.token.str);
				if (var == nullptr) {
					throw Exception("No variable of this name found", statement.token);
				} else if (var->is_param) {
					throw Exception("You may not assign to parameters", statement.token);
				}
				Type t = var->type.merge(type);
				if (!t.is_valid()) {
					throw Exception("Cannot assign " + type.to_string() + " to variable of " +
					                "type " + var->type.to_string(), statement.token);
				}
				var->type = t;
			} break;
			case Statement::EXPR: {
				type_of(*statement.expr, state, statement.token);
			} break;
			case Statement::RETURN: {
				if (state.get_return().get() == Prim::VOID) break;
				Type type = type_of(*statement.expr, state, statement.token);
				Type t = state.get_return().merge(type);
				if (!t.is_valid()) {
					throw Exception("Cannot return " + type.to_string() + "from func returning " +
					                state.get_return().to_string(), statement.token);
				}
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					Type type = type_of(*if_statement.conditions[j], state, if_statement.token);
					if (!type.has(Prim::BOOL)) {
						throw Exception("Condition should be Bool, was " + type.to_string(),
						                if_statement.token);
					}
				}
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					check_types(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				Type type = type_of(*while_statement.condition, state, while_statement.token);
				if (!type.has(Prim::BOOL)) {
					throw Exception("Condition should be Bool, was " + type.to_string(),
					                while_statement.token);
				}
				state.descend(while_statement.block);
				state.create_loop();
				check_types(while_statement.block, state);
				state.ascend();
			} break;
			case Statement::CONTINUE:
				if (state.get_loop(1) == nullptr) {
					throw Exception("No loop to continue", statement.token);
				} break;
			case Statement::BREAK:
				if (state.get_loop(((Break&)statement).amount) == nullptr) {
					throw Exception("No loop to break out of", statement.token);
				} break;
		}
	}
}

Type Checker::type_of(Expr &expr, State& state, const Token& token) {
	static Type unknown;
	std::vector<Type*> stack;
	for (Tok& tok : expr) {
		if (tok.form == Tok::VAR) {
			Variable* var = state.get_var(tok.token.str);
			if (var == nullptr) throw Exception("Variable not found", tok.token);
			stack.push_back(&var->type);
		} else if (tok.form == Tok::FUNCTION) {
			auto& funcs = state.get_functions((int)tok.i, tok.token.str);
			if (funcs.empty()) throw Exception("Function not found", tok.token);
			std::vector<Type> possible(tok.i, Type::invalid());
			std::vector<Function*> valid_funcs;

			std::vector<Type*> arguments(stack.end() - tok.i, stack.end());
			stack.erase(stack.end() - tok.i, stack.end());

			for (Function* func : funcs) {
				if (func->allows(arguments)) {
					for (size_t i = 0; i < tok.i; i++) {
						possible[i].add(func->param_types[i].get());
					}
					valid_funcs.push_back(func);
				}
			}
			if (valid_funcs.empty()) throw Exception("Arguments match no function", tok.token);
			for (size_t i = 0; i < tok.i; i++) {
				*arguments[i] = arguments[i]->merge(possible[i]);
			}
			stack.push_back(valid_funcs.size() == 1 ? &valid_funcs[0]->return_type : &unknown);

		} else if (tok.form == Tok::OP) {
			switch (tok.op) {
				case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
				case Op::BAND: case Op::BOR: case Op::XOR: case Op::LSH: case Op::RSH:
					tok.type = merge_stack(stack, 2, tok.type, tok.token);
					break;
				case Op::AND: case Op::OR:
					merge_stack(stack, 2, Type(Prim::BOOL), tok.token);
					break;
				case Op::GT: case Op::LT: case Op::GEQ: case Op::LEQ:
					merge_stack(stack, 2, NUMBER, tok.token);
					break;
				case Op::EQ: case Op::NEQ:
					merge_stack(stack, 2, Type(), tok.token);
					break;
				case Op::NOT:
					merge_stack(stack, 1, Type(Prim::BOOL), tok.token);
					break;
				case Op::NEG:
					tok.type = merge_stack(stack, 1, tok.type, tok.token);
					if (!tok.type.merge(UNSIGNED).is_valid()) {
						throw Exception("Expected unsigned type", tok.token);
					}
					break;
				case Op::INV:
					tok.type = merge_stack(stack, 1, tok.type, tok.token);
					break;
				case Op::TEMP_PAREN: case Op::TEMP_FUNC: assert(false);
			}
			stack.push_back(&tok.type);
		} else {
			stack.push_back(&tok.type);
		}
	}
	if (stack.empty()) throw Exception("Empty expression.", token);
	return *stack.back();
}

inline void test_type(Type a, Type b, Type t, const Token& token) {
	if (!t.is_valid()) throw Checker::Exception("Types do not match: " + a.to_string() + " and " +
	                                            b.to_string(), token);
}
Type Checker::merge_stack(std::vector<Type*> &stack, int num, Type req_type, const Token &token) {
	if (stack.size() < num) throw Exception("Misused operator.", token);
	Type type;
	for (int i = 0; i < num; i++) {
		Type n =  type.merge(*stack.back());
		test_type(type, *stack.back(), n, token);
		stack.pop_back();
		type = n;
	}
	Type n = type.merge(req_type);
	test_type(type, req_type, n, token);
	return n;
}

void Checker::complete_types(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				complete_types(func.block, state);
			} break;
		}
	}
}

void Checker::complete_types(Block &block, State& state) {
	state.descend(block);
	complete_var_types(block, state);
	state.ascend();
	state.descend(block);
	complete_lit_types(block, state);
	state.ascend();
}

void Checker::complete_var_types(Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& declaration = (Declaration&)statement;
				Variable& var = state.next_var(declaration.token.str);
				var.type.complete();
				if (!var.type.is_known()) {
					throw Exception("Type could not be determined.", declaration.token);
				}
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					complete_var_types(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				state.descend(while_statement.block);
				complete_var_types(while_statement.block, state);
				state.ascend();
			} break;
			default: break;
		}
	}
}
void Checker::complete_lit_types(Block &block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& declaration = (Declaration&)statement;
				Type type = state.next_var(declaration.token.str).type;
				complete_expr(state, *declaration.expr, type);
			} break;
			case Statement::ASSIGNMENT: {
				Type type = state.get_var(statement.token.str)->type;
				complete_expr(state, *statement.expr, type);
			} break;
			case Statement::EXPR: {
				complete_expr(state, *statement.expr, Type());
			} break;
			case Statement::RETURN: {
				Type type = state.get_return();
				if (type.get() != Prim::VOID) complete_expr(state, *statement.expr, type);
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					complete_expr(state, *if_statement.conditions[j], Type(Prim::BOOL));
				}
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					complete_lit_types(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				complete_expr(state, *while_statement.condition, Type(Prim::BOOL));
				state.descend(while_statement.block);
				complete_lit_types(while_statement.block, state);
				state.ascend();
			} break;
			default: break;
		}
	}
}
void Checker::complete_expr(State& state, Expr& expr, Type final_type) {
	std::vector<Type> type_stack;

	// a list of references for each type that the type links to, so when the type is complete
	// all the referenced types are completed as well
	std::vector<std::vector<Type*>> ref_stack;

	for (Tok& tok : expr) {
		switch (tok.form) {
			case Tok::VAR: {
				Type var_type = state.get_var(tok.token.str)->type;
				assert(var_type.is_known());
				type_stack.push_back(var_type);
				ref_stack.emplace_back();
			} break;
			case Tok::FUNCTION: {
				auto& funcs = state.get_functions((int)tok.i, tok.token.str);
				if (funcs.empty()) throw Exception("Function not found", tok.token);
				std::vector<Function*> valid_funcs;

				std::vector<Type>                 args(type_stack.end() - tok.i, type_stack.end());
				std::vector<std::vector<Type*>> ref_args(ref_stack.end() - tok.i, ref_stack.end());
				type_stack.erase(type_stack.end() - tok.i, type_stack.end());
				ref_stack.erase(  ref_stack.end() - tok.i,  ref_stack.end());

				for (Function* func : funcs) {
					if (func->allows(args)) {
						valid_funcs.push_back(func);
					}
				}
				assert(!valid_funcs.empty());
				if (valid_funcs.size() > 2) throw Exception("Couldn't resolve func", tok.token);
				for (size_t i = 0; i < tok.i; i++) {
					if (!args[i].is_known()) {
						for (Type* type : ref_args[i]) {
							*type = valid_funcs[0]->param_types[i];
						}
					}
				}
				tok.something = valid_funcs[0];
				type_stack.push_back(valid_funcs[0]->return_type);
				ref_stack.emplace_back();
			} break;
			case Tok::OP: {
				switch (tok.op) {
					case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
					case Op::AND: case Op::OR: case Op::BAND: case Op::BOR: case Op::XOR:
					case Op::LSH: case Op::RSH: {
						Type type2 = type_stack.back(); type_stack.pop_back();
						auto  vec2 =  ref_stack.back();  ref_stack.pop_back();
						Type type1 = type_stack.back(); type_stack.pop_back();
						auto  vec1 =  ref_stack.back();  ref_stack.pop_back();

						if (!type1.is_known()) {
							if (!type2.is_known()) {
								vec1.insert(vec1.end(), vec2.begin(), vec2.end());
								type_stack.push_back(Type());
								ref_stack.push_back(vec1);
							} else {
								for (Type* type : vec1) *type = type2;
								type_stack.push_back(type2);
								ref_stack.emplace_back();
							}
						} else {
							if (!type2.is_known()) {
								for (Type* type : vec2) *type = type1;
							}
							type_stack.push_back(type1);
							ref_stack.emplace_back();
						}
					} break;
					case Op::GT: case Op::LT: case Op::GEQ: case Op::LEQ:
					case Op::EQ: case Op::NEQ: {
						Type type2 = type_stack.back(); type_stack.pop_back();
						auto  vec2 =  ref_stack.back();  ref_stack.pop_back();
						Type type1 = type_stack.back(); type_stack.pop_back();
						auto  vec1 =  ref_stack.back();  ref_stack.pop_back();

						if (!type1.is_known()) {
							if (!type2.is_known()) {
								for (Type* type : vec1) type->complete();
								for (Type* type : vec2) type->complete();
							} else {
								for (Type* type : vec1) *type = type2;
							}
						} else {
							for (Type* type : vec2) *type = type1;
						}
						type_stack.push_back(Type(Prim::BOOL));
						ref_stack.emplace_back();
					} break;
					case Op::NOT: case Op::NEG: break;
					default: assert(false); break;
				}
			} break;
			default:
				ref_stack.emplace_back();
				if (!tok.type.is_known()) {
					type_stack.push_back(Type());
					ref_stack.back().push_back(&tok.type);
				} else {
					type_stack.push_back(tok.type);
				}
		}
	}
	if (!type_stack.back().is_known()) {
		for (Type* type : ref_stack.back()) *type = final_type;
	}
}



Checker::Exception::Exception(std::string desc, Token token):
		std::invalid_argument(desc), desc(desc), token(token) {
	std::stringstream ss;
	ss << "line " << token.line << " (" << token.str << "): " << desc;
	this->desc = ss.str();
}
const char* Checker::Exception::what() const throw() {
	return desc.c_str();
}
