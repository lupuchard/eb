#include <passes/TypeChecker.h>
#include <passes/LitCompleter.h>
#include <passes/VarCompleter.h>
#include <passes/ReturnChecker.h>
#include "passes/Passer.h"

void Passer::pass(Module& module, State& state) {
	// Insures all paths return a value and adds implicit returns when necessary.
	ReturnChecker return_checker;
	return_checker.check(module);

	// Infers as much type information as possible.
	// Fails if any types don't match.
	TypeChecker type_checker;
	type_checker.check(module, state);

	// Numerically ambiguous variables are set to either I32 or F64.
	// Fails if any types are still unknown.
	VarCompleter var_completer;
	var_completer.complete(module, state);

	// Ambiguous literals are fixed.
	LitCompleter lit_completer;
	lit_completer.complete(module, state);
}
