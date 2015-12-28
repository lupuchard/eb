#ifndef EBC_CHECKER_H
#define EBC_CHECKER_H

#include "ast/Module.h"
#include "State.h"
#include "ast/Statement.h"

class TypeChecker {
public:
	void check(Module& module, State& state);
	void check(Block& block, State& state);
	Type type_of(Expr& expr, State& state, const Token& token);

private:
	Type merge_stack(std::vector<Type*>& stack, int num, Type type, const Token& token);
};


#endif //EBC_CHECKER_H
