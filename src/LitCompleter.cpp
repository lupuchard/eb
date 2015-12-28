#include "passes/LitCompleter.h"
#include "Exception.h"
#include <cassert>

void LitCompleter::complete(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				state.descend(func.block);
				complete(func.block, state);
				state.ascend();
			} break;
		}
	}
}

void LitCompleter::complete(Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& declaration = (Declaration&)statement;
				Type type = state.next_var(declaration.token.str).type;
				if (declaration.expr != nullptr) complete(*declaration.expr, state, type);
			} break;
			case Statement::ASSIGNMENT: {
				Type type = state.get_var(statement.token.str)->type;
				complete(*statement.expr, state, type);
			} break;
			case Statement::EXPR: {
				complete(*statement.expr, state, Type());
			} break;
			case Statement::RETURN: {
				Type type = state.get_return();
				if (type.get() != Prim::VOID) complete(*statement.expr, state, type);
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (size_t j = 0; j < if_statement.conditions.size(); j++) {
					complete(*if_statement.conditions[j], state, Type(Prim::BOOL));
				}
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					complete(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				complete(*while_statement.condition, state, Type(Prim::BOOL));
				state.descend(while_statement.block);
				complete(while_statement.block, state);
				state.ascend();
			} break;
			default: break;
		}
	}
}

void LitCompleter::complete(Expr& expr, State& state, Type final_type) {
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
