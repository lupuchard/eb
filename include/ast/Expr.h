#ifndef EBC_EXPR_H
#define EBC_EXPR_H

#include "Token.h"
#include "Variable.h"
#include <vector>
#include <memory>

enum class Op {
	ADD, SUB, MUL, DIV, MOD, NEG, INV,
	BAND, BOR, XOR, LSH, RSH,
	NOT, AND, OR, GT, LT, GEQ, LEQ, EQ, NEQ,
	TEMP_PAREN, TEMP_FUNC,
};
inline bool is_binary(Op op) {
	switch (op) {
		case Op::NEG: case Op::INV: case Op::NOT: return false;
		default: return true;
	}
}

struct Tok {
	enum Form { INT, FLOAT, OP, VAR, FUNC, IF };

	Tok(const Token& token, Form form, Type type = Type()) :
			token(&token), form(form), type(type) { }

	virtual void _() const { }

	const Token* token;
	Form form;
	Type type;
};

struct IntTok: public Tok {
	IntTok(const Token& token, uint64_t i, Type type): Tok(token, INT, type), i(i) { }
	IntTok(const Token& token, bool b): Tok(token, INT, Type(Prim::BOOL)), i((uint64_t)b) { }
	uint64_t i;
};

struct FloatTok: public Tok {
	FloatTok(const Token& token, double f, Type type): Tok(token, FLOAT, type), f(f) { }
	double f;
};

struct OpTok: public Tok {
	OpTok(const Token& token, Op op, Type type = Type()): Tok(token, OP, type), op(op) { }
	Op op;
};

struct VarTok: public Tok {
	VarTok(const Token& token): Tok(token, VAR) { }
	Variable* var = nullptr;
};

typedef std::vector<std::unique_ptr<Tok>> Expr;

#endif //EBC_EXPR_H
