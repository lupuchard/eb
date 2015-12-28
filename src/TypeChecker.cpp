#include "passes/TypeChecker.h"
#include "Exception.h"
#include <cassert>
#include <iostream>

void TypeChecker::check(Module& module, State& state) {
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
				check(func.block, state);
				state.ascend();
			} break;
		}
	}
}

void TypeChecker::check(Block& block, State& state) {
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
					check(if_block, state);
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
				check(while_statement.block, state);
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

Type TypeChecker::type_of(Expr &expr, State& state, const Token& token) {
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
	if (!t.is_valid()) throw Exception("Types do not match: " + a.to_string() +
	                                   " and " + b.to_string(), token);
}
Type TypeChecker::merge_stack(std::vector<Type*>& stack, int num, Type req_type, const Token& t) {
	if (stack.size() < num) throw Exception("Misused operator.", t);
	Type type;
	for (int i = 0; i < num; i++) {
		Type n =  type.merge(*stack.back());
		test_type(type, *stack.back(), n, t);
		stack.pop_back();
		type = n;
	}
	Type n = type.merge(req_type);
	test_type(type, req_type, n, t);
	return n;
}
