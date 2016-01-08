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
			token(token), var(type), conzt(conzt), Item(GLOBAL) { }
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

	inline bool allows(const std::vector<Type*>& arguments) {
		for (size_t i = 0; i < arguments.size(); i++) {
			if (!arguments[i]->has(param_types[i].get())) {
				return false;
			}
		}
		return true;
	}
	inline bool allows(const std::vector<Type>& arguments) {
		for (size_t i = 0; i < arguments.size(); i++) {
			if (!arguments[i].has(param_types[i].get())) {
				return false;
			}
		}
		return true;
	}

	const Token& token;
	Block block;
	Type return_type = Type(Prim::VOID);
	int index = 0;
	std::vector<Type> param_types;
	std::vector<const Token*> param_names;
	std::string unique_name;
};
struct FuncTok: public Tok {
	FuncTok(const Token& token, int num_params): Tok(token, FUNC), num_params(num_params) { }
	int num_params;
	std::vector<Function*> possible_funcs;
	bool external = false;
};

#endif //EBC_ITEM_H
