#ifndef EBC_VARCOMPLETER_H
#define EBC_VARCOMPLETER_H

#include <State.h>
#include <ast/Module.h>

class VarCompleter {
public:
	void complete(Module& module, State& state);
	void complete(Block& block, State& state);
};


#endif //EBC_VARCOMPLETER_H
