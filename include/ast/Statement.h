#ifndef EBC_STATEMENT_H
#define EBC_STATEMENT_H

#include "Expr.h"
#include <memory>

struct Statement {
	enum Form { DECLARATION, ASSIGNMENT, RETURN, IF, WHILE, CONTINUE, BREAK };
	Form form;
	const Token& token;
	Statement(const Token& token, Form form): token(token), form(form) { }
	virtual void _() const { } // MOST USEFUL METHOD DOES ALL THINGS
};

typedef std::vector<std::unique_ptr<Statement>> Block;

struct Declaration: public Statement {
	Declaration(const Token& token):
			Statement(token, DECLARATION) { }
	Declaration(const Token& token, const Token& type_token):
			Statement(token, DECLARATION), type_token(&type_token) { }
	const Token* type_token = nullptr;
	std::unique_ptr<Expr> value;
};

struct Assignment: public Statement {
	Assignment(const Token& token): Statement(token, ASSIGNMENT) { }
	std::unique_ptr<Expr> value;
};

struct Return: public Statement {
	Return(const Token& token): Statement(token, RETURN) { }
	std::unique_ptr<Expr> value;
};

struct If: public Statement {
	If(const Token& token): Statement(token, IF) { }
	std::vector<std::unique_ptr<Expr>> conditions;
	std::vector<Block> blocks;
};

struct While: public Statement {
	While(const Token& token): Statement(token, WHILE) { }
	std::unique_ptr<Expr> condition;
	Block block;
};

struct Break: public Statement {
	Break(const Token& token): Statement(token, BREAK) { }
	int amount = 1;
};

#endif //EBC_STATEMENT_H
