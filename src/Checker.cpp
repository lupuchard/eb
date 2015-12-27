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
				if (exists) throw Exception("Duplicate function name.", func.name_token);
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
				Type type = Type::UNKNOWN;
				if (decl.type_token != nullptr) {
					type = parse_type(decl.type_token->str);
				}
				if (decl.value.get() != nullptr) {
					type = merge_types(type, type_of(*decl.value, state, decl.token));
				}
				state.declare(decl.token.str, type);
			} break;
			case Statement::ASSIGNMENT: {
				Assignment& assign = (Assignment&)statement;
				Type type = type_of(*assign.value, state, assign.token);
				Variable* var = state.get_var(assign.token.str);
				if (var == nullptr) {
					throw Exception("No variable of this name found", assign.token);
				} else if (var->is_param) {
					throw Exception("You may not assign to parameters", assign.token);
				}
				Type t = merge_types(var->type, type);
				if (t == Type::INVALID) {
					throw Exception("Cannot assign " + type_to_string(type) + " to variable of " +
					                "type " + type_to_string(var->type), assign.token);
				}
				var->type = t;
			} break;
			case Statement::RETURN: {
				Return& ret = (Return&)statement;
				if (state.get_return() == Type::VOID) break;
				Type type = type_of(*ret.value, state, ret.token);
				Type t = merge_types(state.get_return(), type);
				if (t == Type::INVALID) {
					throw Exception("Cannot return " + type_to_string(type) + "from function " +
					                "returning " + type_to_string(state.get_return()), ret.token);
				}
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					Type type = type_of(*if_statement.conditions[j], state, if_statement.token);
					if (merge_types(Type::BOOL, type) == Type::INVALID) {
						throw Exception("Condition should be Bool, was " + type_to_string(type),
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
				if (merge_types(Type::BOOL, type) == Type::INVALID) {
					throw Exception("Condition should be Bool, was " + type_to_string(type),
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
	std::vector<Type> stack;
	for (Tok& tok : expr) {
		if (tok.form == Tok::VAR) {
			Variable* var = state.get_var(tok.token.str);
			if (var == nullptr) throw Exception("Variable not found", tok.token);
			stack.push_back(var->type);
		} else if (tok.form == Tok::FUNCTION) {
			Function* func = state.get_func(tok.token.str);
			if (func == nullptr) throw Exception("Function not found", tok.token);
			for (int i = (int)func->param_types.size() - 1; i >= 0; i--) {
				Type arg = stack.back();
				stack.pop_back();
				if (merge_types(arg, func->param_types[i]) == Type::INVALID) {
					throw Exception("Can't pass " + type_to_string(arg) + " as argument of type " +
					                type_to_string(func->param_types[i]), tok.token);
				}
			}
			stack.push_back(func->return_type);
		} else if (tok.form == Tok::OP) {
			switch (tok.op) {
				case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
				case Op::BAND: case Op::BOR: case Op::XOR: case Op::LSH: case Op::RSH:
					tok.type = merge_stack(stack, 2, tok.type, tok.token);
					break;
				case Op::AND: case Op::OR:
					merge_stack(stack, 2, Type::BOOL, tok.token);
					break;
				case Op::GT: case Op::LT: case Op::GEQ: case Op::LEQ:
					merge_stack(stack, 2, Type::NUM, tok.token);
					break;
				case Op::EQ: case Op::NEQ:
					merge_stack(stack, 2, Type::UNKNOWN, tok.token);
					break;
				case Op::NOT:
					merge_stack(stack, 1, Type::BOOL, tok.token);
					break;
				case Op::NEG:
					tok.type = merge_stack(stack, 1, tok.type, tok.token);
					if (is_type(tok.type, Type::UNSIGNED)) {
						throw Exception("Expected unsigned type", tok.token);
					}
					break;
				case Op::INV:
					tok.type = merge_stack(stack, 1, tok.type, tok.token);
					break;
				case Op::TEMP_PAREN: case Op::TEMP_FUNC: assert(false);
			}
			stack.push_back(tok.type);
		} else {
			stack.push_back(tok.type);
		}
	}
	if (stack.empty()) throw Exception("Empty expression.", token);
	return stack.back();
}

inline void test_type(Type a, Type b, Type t, const Token& token) {
	if (t == Type::INVALID) throw Checker::Exception("Types do not match: " +
	                                                 type_to_string(a) + " and " +
	                                                 type_to_string(b), token);
}
Type Checker::merge_stack(std::vector<Type> &stack, int num, Type req_type, const Token &token) {
	if (stack.size() < num) throw Exception("Misused operator.", token);
	Type type = Type::UNKNOWN;
	for (int i = 0; i < num; i++) {
		Type n = merge_types(type, stack.back());
		test_type(type, stack.back(), n, token);
		stack.pop_back();
		type = n;
	}
	Type n = merge_types(type, req_type);
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
				var.type = complete_type(var.type);
				if (var.type == Type::INVALID) {
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
				complete_expr(state, *declaration.value, type);
			} break;
			case Statement::ASSIGNMENT: {
				Assignment& assignment = (Assignment&)statement;
				Type type = state.get_var(assignment.token.str)->type;
				complete_expr(state, *assignment.value, type);
			} break;
			case Statement::RETURN: {
				Return& return_statement = (Return&)statement;
				Type type = state.get_return();
				if (type != Type::VOID) complete_expr(state, *return_statement.value, type);
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					complete_expr(state, *if_statement.conditions[j], Type::BOOL);
				}
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					complete_lit_types(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				complete_expr(state, *while_statement.condition, Type::BOOL);
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
				assert(var_type == complete_type(var_type) && var_type != Type::INVALID);
				type_stack.push_back(var_type);
				ref_stack.emplace_back();
			} break;
			case Tok::FUNCTION: {
				Function* function = state.get_func(tok.token.str);
				for (Type param : function->param_types) {
					if (type_stack.back() == Type::UNKNOWN) {
						for (Type* type : ref_stack.back()) {
							*type = param;
						}
					}
					type_stack.pop_back();
					ref_stack.pop_back();
				}
				type_stack.push_back(function->return_type);
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

						if (type1 == Type::UNKNOWN) {
							if (type2 == Type::UNKNOWN) {
								vec1.insert(vec1.end(), vec2.begin(), vec2.end());
								type_stack.push_back(Type::UNKNOWN);
								ref_stack.push_back(vec1);
							} else {
								for (Type* type : vec1) *type = type2;
								type_stack.push_back(type2);
								ref_stack.emplace_back();
							}
						} else {
							if (type2 == Type::UNKNOWN) {
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

						if (type1 == Type::UNKNOWN) {
							if (type2 == Type::UNKNOWN) {
								for (Type* type : vec1) *type = complete_type(*type);
								for (Type* type : vec2) *type = complete_type(*type);
							} else {
								for (Type* type : vec1) *type = type2;
							}
						} else {
							for (Type* type : vec2) *type = type1;
						}
						type_stack.push_back(Type::BOOL);
						ref_stack.emplace_back();
					} break;
					case Op::NOT: case Op::NEG: break;
					default: assert(false); break;
				}
			} break;
			default:
				ref_stack.emplace_back();
				if (tok.type != complete_type(tok.type)) {
					type_stack.push_back(Type::UNKNOWN);
					ref_stack.back().push_back(&tok.type);
				} else {
					type_stack.push_back(tok.type);
				}
		}
	}
	if (type_stack.back() == Type::UNKNOWN) {
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
