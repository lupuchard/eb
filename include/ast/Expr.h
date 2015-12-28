#ifndef EBC_EXPR_H
#define EBC_EXPR_H

#include "Token.h"
#include <vector>

enum class Op {
	ADD, SUB, MUL, DIV, MOD, NEG, INV,
	BAND, BOR, XOR, LSH, RSH,
	NOT, AND, OR, GT, LT, GEQ, LEQ, EQ, NEQ,
	TEMP_PAREN, TEMP_FUNC,
};

struct Tok {
	enum Form { INT_LIT, FLOAT_LIT, BOOL_LIT, OP, VAR, FUNCTION };

	Tok(const Token& token, Form form, Type type = Type()):
			token(token), form(form), type(type) { }
	Tok(const Token& token, Op op, Type type = Type()):
			token(token), form(OP), op(op), type(type) { }
	Tok(const Token& token, uint64_t i, Type type):
			form(INT_LIT), token(token), i(i), type(type) { }
	Tok(const Token& token, double f,   Type type):
			form(FLOAT_LIT), token(token), f(f), type(type) { }
	Tok(const Token& token, bool b):
			form(BOOL_LIT), token(token), b(b), type(Prim::BOOL) { }

	const Token& token;
	Type type;

	Form form;
	union {
		uint64_t i;
		double f;
		bool b;
		Op op;
		void* something;
	};
};

typedef std::vector<Tok> Expr;

#endif //EBC_EXPR_H
