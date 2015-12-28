#ifndef EBC_PASSER_H
#define EBC_PASSER_H

#include "ast/Module.h"
#include "State.h"

class Passer {
public:
	void pass(Module& module, State& state);
};


#endif //EBC_PASSER_H
