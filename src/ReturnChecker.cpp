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
			case Item::GLOBAL: break;
		}
	}
}

bool ReturnChecker::check(Block& block) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::IF: {
				If& if_statement = (If&)statement;
				if_statement.true_returns = check(if_statement.true_block);
				if_statement.else_returns = check(if_statement.else_block);
				if (if_statement.true_returns && if_statement.else_returns) {
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
			if (!if_statement.true_returns) create_implicit_returns(if_statement.true_block);
			if (!if_statement.else_returns) create_implicit_returns(if_statement.else_block);
		} break;
		default: throw Exception("Expected return", statement.token);
	}
}

void ReturnChecker::create_drops(Block* block) {
	Block new_block;
	for (size_t i = 0; i < block->size(); i++) {
		Statement& statement = *block->at(i);
		if (!statement.expr.empty()) {
			std::vector<Statement*> new_statements;
			create_drops(statement.expr, new_statements);
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
				create_drops(&if_statement.true_block);
				create_drops(&if_statement.else_block);
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
		Tok& tok = *expr[i];
		switch (tok.form) {
			case Tok::IF: {
				If& if_statement = *((IfTok&)tok).if_statement;
				std::stringstream ss;
				ss << "eb$tmp" << index++;
				Token* t = new Token(Token::IDENT, ss.str(), tok.token->line, tok.token->column);
				phantom_tokens.emplace_back(std::unique_ptr<Token>(t));
				Declaration* decl = new Declaration(*t);
				new_statements.push_back(decl);
				new_statements.push_back(&if_statement);
				create_drop(if_statement.true_block, if_statement.token, *t);
				create_drop(if_statement.else_block, if_statement.token, *t);
				expr[i].reset(new VarTok(*t));
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
			create_drop(if_statement.true_block, if_statement.token, tmp);
			create_drop(if_statement.else_block, if_statement.token, tmp);
		} break;
		default: break;
	}
}
