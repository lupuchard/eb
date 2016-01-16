#ifndef EBC_EXPR_H
#define EBC_EXPR_H

#include "Token.h"
#include "Variable.h"
#include <vector>
#include <memory>
#include <map>

struct Tok {
	enum Form { VALUE, VAR, FUNC, IF, ACCESS };

	Tok(const Token& token, Form form) :
			token(&token), form(form) { }

	virtual void _() const { }

	const Token* token;
	Form form;
};


struct Value {
	Value() { }
	Value(bool val): type(Type::Bool), integer((uint64_t)val) { }
	Value(int64_t val,  Type type = Type::Invalid): type(type), integer((uint64_t)val) { }
	Value(uint64_t val, Type type = Type::Invalid): type(type), integer(val) { }
	Value(double val,   Type type = Type::Invalid): type(type), flt(val) { }

	bool b(const Token& token) const;
	bool b() const;
	uint64_t i(const Token& token) const;
	uint64_t i() const;
	double f(const Token& token) const;
	double f() const;

	Type type = Type::Invalid;
	union {
		uint64_t integer;
		double flt;
	};
};
struct ValueTok: public Tok {
	ValueTok(const Token& token, Value value): Tok(token, VALUE), value(value) {
		if (value.type == Type::Invalid) this->value.type = Type(token.suffix);
	}
	Value value;
};


struct VarTok: public Tok {
	VarTok(const Token& token): Tok(token, VAR) { }
	Variable* var = nullptr;
	bool external = false;
};

struct AccessTok: public Tok {
	AccessTok(const Token& token, std::string name): Tok(token, ACCESS), name(name) { }
	int idx;
	std::string name;
};


typedef std::vector<std::unique_ptr<Tok>> Expr;

void insert(Expr* expr, std::vector<std::pair<Tok*, Tok*>>& insertions);
void insert(Expr* expr, std::map<Tok*, Tok*>& insertions);

#endif //EBC_EXPR_H
