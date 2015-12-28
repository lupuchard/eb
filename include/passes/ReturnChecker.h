#ifndef EBC_RETURNCHECKER_H
#define EBC_RETURNCHECKER_H

#include "State.h"
#include "ast/Module.h"

class ReturnChecker {
public:
	void check(Module& module);
	bool check(Block& block);
	void create_implicit_returns(Block& block);
};


#endif //EBC_RETURNCHECKER_H
