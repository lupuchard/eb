#ifndef EBC_CHECKER_H
#define EBC_CHECKER_H

#include "ast/Module.h"
#include "State.h"
#include "ast/Statement.h"
#include "Std.h"

class TypeChecker {
public:
	TypeChecker(Std& std);
	void check(Module& module, State& state);

private:
	void check(Block& block, State& state);
	Type check(Expr* expr, State& state, const Token& token, Type res = Type::Invalid);

	Std& std;
};


#endif //EBC_CHECKER_H
