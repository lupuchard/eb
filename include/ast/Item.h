#ifndef EBC_ITEM_H
#define EBC_ITEM_H

#include "Statement.h"
#include "Variable.h"
#include "StaticEval.h"
#include <unordered_map>

struct Item {
	enum Form { MODULE, FUNCTION, GLOBAL, IMPORT, STRUCT };
	Item(Form form, const Token& token): form(form), token(token) { }
	Form form;
	bool pub = false;
	const Token& token;
	virtual void _() const { }
};

struct Global: public Item {
	Global(const Token& token, Type type, Value val, bool conzt):
			var(type), val(val), conzt(conzt), Item(GLOBAL, token) {
		var.is_const = conzt;
	}
	Variable var;
	Value val;
	bool conzt;
	std::string unique_name;
};

struct Import: public Item {
	Import(const Token& token): Item(IMPORT, token) { }
	std::vector<const Token*> target;
};

struct Function: public Item {
	Function(const Token& token): Item(FUNCTION, token) { }

	Block block;
	Type return_type = Type::Void;
	int index = 0;

	std::vector<Type> param_types;
	std::vector<const Token*> param_names;

	std::vector<Type>  named_param_types;
	std::vector<Value> named_param_vals;
	std::vector<const Token*> named_param_names;
	std::map<std::string, int> named_param_map;

	inline void add_param(const Token& name, Type type) {
		param_types.push_back(type);
		param_names.push_back(&name);
	}
	inline void add_named_param(const Token& name, Type type, Value val = Value()) {
		named_param_map[name.str()] = (int)named_param_types.size();
		named_param_types.push_back(type);
		named_param_names.push_back(&name);
		named_param_vals.push_back(val);
	}

	std::string unique_name;

	enum Form { USER, OP, CAST, CONSTRUCTOR };
	Form form = USER;
};
struct FuncTok: public Tok {
	FuncTok(const Token& token, int num_args):
			Tok(token, FUNC), num_args(num_args), num_unnamed_args(num_args) { }
	int num_args;
	int num_unnamed_args;
	std::vector<std::string> named_args;
	std::vector<Function*> possible_funcs;
	bool external = false;
};

struct Struct: public Item {
	Struct(const Token& token): Item(STRUCT, token) { }

	std::vector<Type> member_types;
	std::vector<const Token*> member_names;
	std::unordered_map<std::string, int> member_map;
	std::string unique_name;
	std::unique_ptr<Function> constructor;

	inline void add_member(const Token& token, Type type) {
		member_map[token.str()] = member_names.size();
		member_names.push_back(&token);
		member_types.push_back(type);
	}
};

#endif //EBC_ITEM_H
