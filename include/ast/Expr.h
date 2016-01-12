#ifndef EBC_EXPR_H
#define EBC_EXPR_H

#include "Token.h"
#include "Variable.h"
#include <vector>
#include <memory>

struct Tok {
	enum Form { INT, FLOAT, VAR, FUNC, IF };

	Tok(const Token& token, Form form, Type type = Type::Invalid) :
			token(&token), form(form), type(type) { }

	virtual void _() const { }

	const Token* token;
	Form form;
	Type type;
};

struct IntTok: public Tok {
	IntTok(const Token& token, uint64_t i, Type type): Tok(token, INT, type), i(i) { }
	IntTok(const Token& token, bool b): Tok(token, INT, Type::Bool), i((uint64_t)b) { }
	uint64_t i;
};

struct FloatTok: public Tok {
	FloatTok(const Token& token, double f, Type type): Tok(token, FLOAT, type), f(f) { }
	double f;
};

struct VarTok: public Tok {
	VarTok(const Token& token): Tok(token, VAR) { }
	Variable* var = nullptr;
	bool external = false;
};

typedef std::vector<std::unique_ptr<Tok>> Expr;

#endif //EBC_EXPR_H
