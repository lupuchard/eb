#include "passes/Passer.h"

void Passer::pass(Module& module, State& state) {
	// Insures all paths return a value and adds implicit returns when necessary.
	// Transforms assignment ifs.
	return_checker.check(module);

	// Infers as much type information as possible.
	// Fails if any types don't match.
	// Checks validity of breaks/continues for some reason.
	type_checker.check(module, state);

	// Numerically ambiguous variables are set to either I32 or F64.
	// Fails if any types are still unknown.
	var_completer.complete(module, state);

	// Ambiguous literals are fixed.
	lit_completer.complete(module, state);
}
