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
	void check(Module& mod, Block& block, State& state);
	Type check(Module& mod, Expr* expr, State& state, const Token& token, Type res = Type::Invalid);

	Std& std;

	void insert_cast(const Token& token, std::map<Tok*, Tok*>& insertions, Tok* tok, Type arg,
	                 Type param);
};


#endif //EBC_CHECKER_H
