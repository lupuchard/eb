#include "passes/ReturnChecker.h"
#include "Exception.h"

void ReturnChecker::check(Module& module) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				if (func.return_type.get() != Prim::VOID) {
					if (!check(func.block)) {
						create_implicit_returns(func.block);
					}
				}
			} break;
		}
	}
}

bool ReturnChecker::check(Block& block) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::IF: {
				If& if_statement = (If&) statement;
				if_statement.returns = std::vector<bool>(if_statement.blocks.size(), false);
				bool fully_returns = true;
				for (size_t j = 0; j < if_statement.blocks.size(); j++) {
					if_statement.returns[j] = check(if_statement.blocks[j]);
					fully_returns = fully_returns && if_statement.returns[j];
				}
				if (fully_returns && if_statement.blocks.size() > if_statement.conditions.size()) {
					if (i != block.size() - 1) {
						throw Exception("Unreachable code after if", statement.token);
					}
					return true;
				}
			} break;
			case Statement::RETURN:
				if (i != block.size() - 1) {
					throw Exception("Unreachable code after return", statement.token);
				}
				return true;
			default: break;
		}
	}
	return false;
}

void ReturnChecker::create_implicit_returns(Block& block) {
	if (block.empty()) return;
	Statement& statement = *block.back();
	switch (statement.form) {
		case Statement::EXPR: {
			std::unique_ptr<Statement> ret(new Statement(statement.token, Statement::RETURN));
			ret->expr.swap(statement.expr);
			block.back().swap(ret);
		} break;
		case Statement::IF: {
			If& if_statement = (If&) statement;
			if (if_statement.blocks.size() == if_statement.conditions.size()) {
				throw Exception("Expected return after if", if_statement.token);
			}
			for (size_t i = 0; i < if_statement.blocks.size(); i++) {
				if (!if_statement.returns[i]) {
					create_implicit_returns(if_statement.blocks[i]);
				}
			}
		} break;
		default: throw Exception("Expected return", statement.token);
	}
}
