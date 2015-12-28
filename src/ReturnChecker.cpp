#include "passes/ReturnChecker.h"
#include "Exception.h"

void ReturnChecker::check(Module& module) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				create_drops(&func.block);
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
				If& if_statement = (If&)statement;
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
				if (i < block.size() - 1) {
					throw Exception("Unreachable code", block[i + 1]->token);
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

void ReturnChecker::create_drops(Block* block) {
	Block new_block;
	for (size_t i = 0; i < block->size(); i++) {
		Statement& statement = *block->at(i);
		if (statement.expr != nullptr) {
			std::vector<Statement*> new_statements;
			create_drops(*statement.expr, new_statements);
			for (Statement* new_statement : new_statements) {
				new_block.push_back(std::unique_ptr<Statement>(new_statement));
			}
		}
		new_block.emplace_back();
		new_block.back().swap(block->at(i));
	}
	*block = std::move(new_block);
	for (size_t i = 0; i < block->size(); i++) {
		Statement& statement = *block->at(i);
		switch (statement.form) {
			case Statement::IF: {
				If& if_statement = (If&)statement;
				for (Block& if_block : if_statement.blocks) {
					create_drops(&if_block);
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				create_drops(&while_statement.block);
			} break;
			default: break;
		}
	}
}

void ReturnChecker::create_drops(Expr& expr, std::vector<Statement*>& new_statements) {
	for (size_t i = 0; i < expr.size(); i++) {
		Tok& tok = expr[i];
		switch (tok.form) {
			case Tok::IF: {
				If& if_statement = *(If*)tok.something;
				if (if_statement.blocks.size() == if_statement.conditions.size()) {
					throw Exception("Assigned ifs must have an else", tok.token);
				}

				std::stringstream ss;
				ss << "eb$tmp" << index++;
				Token* t = new Token(Token::IDENT, ss.str(), tok.token.line, tok.token.column);
				phantom_tokens.emplace_back(std::unique_ptr<Token>(t));
				Declaration* decl = new Declaration(*t);
				new_statements.push_back(decl);
				new_statements.push_back(&if_statement);

				for (size_t j = 0; j < if_statement.blocks.size(); j++) {
					create_drop(if_statement.blocks[j], if_statement.token, *t);
				}

				expr[i] = Tok(*t, Tok::VAR);
			} break;
			default: break;
		}
	}
}

void ReturnChecker::create_drop(Block& block, const Token& token, Token& tmp) {
	if (block.empty()) throw Exception("Expected drop from block", token);
	Statement& statement = *block.back();
	switch (statement.form) {
		case Statement::EXPR: {
			std::unique_ptr<Statement> ass(new Statement(tmp, Statement::ASSIGNMENT));
			ass->expr.swap(statement.expr);
			block.back().swap(ass);
		} break;
		case Statement::IF: {
			If& if_statement = (If&)statement;
			if (if_statement.blocks.size() == if_statement.conditions.size()) {
				throw Exception("Dropping if must have an else", if_statement.token);
			}
			for (size_t j = 0; j < if_statement.blocks.size(); j++) {
				create_drop(if_statement.blocks[j], if_statement.token, tmp);
			}
		} break;
		default: break;
	}
}
