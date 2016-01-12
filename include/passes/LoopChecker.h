#ifndef EBC_LOOPCHECKER_H
#define EBC_LOOPCHECKER_H

#include "ast/Module.h"
#include "State.h"

class LoopChecker {
public:
	void check(Module& module, State& state);

private:
	void check(Block& block, State& state);
};


#endif //EBC_LOOPCHECKER_H
