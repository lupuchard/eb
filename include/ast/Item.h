#ifndef EBC_ITEM_H
#define EBC_ITEM_H

#include "Statement.h"

struct Item {
	enum Form { FUNCTION };
	Item(Form form): form(form) { }
	Form form;
	virtual void _() const { }
};

struct Function: public Item {
	Function(const Token& name_token): name_token(name_token), Item(FUNCTION) { }

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

	const Token& name_token;
	Block block;
	Type return_type = Type(Prim::VOID);
	int index = 0;
	std::vector<Type> param_types;
	std::vector<const Token*> param_names;
};

typedef std::vector<std::unique_ptr<Item>> Module;

#endif //EBC_ITEM_H
