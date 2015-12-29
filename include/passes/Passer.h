#ifndef EBC_PASSER_H
#define EBC_PASSER_H

#include "ast/Module.h"
#include "State.h"
#include "passes/Circuiter.h"
#include "passes/TypeChecker.h"
#include "passes/LitCompleter.h"
#include "passes/VarCompleter.h"
#include "passes/ReturnChecker.h"

class Passer {
public:
	void pass(Module& module, State& state);

private:
	Circuiter circuiter;
	ReturnChecker return_checker;
	TypeChecker type_checker;
	VarCompleter var_completer;
	LitCompleter lit_completer;
};


#endif //EBC_PASSER_H
