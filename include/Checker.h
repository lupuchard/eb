#ifndef EBC_CHECKER_H
#define EBC_CHECKER_H


#include <ast/Item.h>
#include "State.h"
#include "ast/Statement.h"

class Checker {
public:

	struct Exception: std::invalid_argument {
		Exception(std::string desc, Token token);
		virtual const char* what() const throw();
		std::string desc;
		Token token;
	};

	void check_types(Module& module, State& state);
	void check_types(Block& block, State& state);
	Type type_of(Expr& expr, State& state, const Token& token);

	void complete_types(Module& module, State& state);
	void complete_types(Block& block, State& state);

private:
	Type merge_stack(std::vector<Type>& stack, int num, Type type, const Token& token);

	void complete_var_types(Block& block, State& state);
	void complete_lit_types(Block& block, State& state);
	void complete_expr(State& state, Expr& expr, Type ret_type);
};


#endif //EBC_CHECKER_H
