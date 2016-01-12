#ifndef EBC_ITEM_H
#define EBC_ITEM_H

#include "Statement.h"
#include "Variable.h"

struct Item {
	enum Form { FUNCTION, GLOBAL, IMPORT };
	Item(Form form): form(form) { }
	Form form;
	bool pub = false;
	virtual void _() const { }
};

struct Global: public Item {
	Global(const Token& token, Type type, bool conzt):
			token(token), var(type), conzt(conzt), Item(GLOBAL) {
		var.is_const = conzt;
	}
	const Token& token;
	Variable var;
	bool conzt;
	std::string unique_name;
};

struct Import: public Item {
	Import(const Token& token): token(token), Item(IMPORT) { }
	const Token& token;
	std::vector<const Token*> target;
};

struct Function: public Item {
	Function(const Token& token): token(token), Item(FUNCTION) { }

	const Token& token;
	Block block;
	Type return_type = Type::Void;
	int index = 0;
	std::vector<Type> param_types;
	std::vector<const Token*> param_names;
	std::string unique_name;

	enum Form { USER, OP, CAST };
	Form form = USER;
};
struct FuncTok: public Tok {
	FuncTok(const Token& token, int num_params): Tok(token, FUNC), num_params(num_params) { }
	int num_params;
	std::vector<Function*> possible_funcs;
	bool external = false;
};

#endif //EBC_ITEM_H
