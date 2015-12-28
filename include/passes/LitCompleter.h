#ifndef EBC_LITCOMPLETER_H
#define EBC_LITCOMPLETER_H

#include "ast/Module.h"
#include "State.h"

class LitCompleter {
public:
	void complete(Module& module, State& state);
	void complete(Block& block, State& state);
	void complete(Expr& expr, State& state, Type type);
};


#endif //EBC_LITCOMPLETER_H
