#include "passes/VarCompleter.h"
#include "Exception.h"

void VarCompleter::complete(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				complete(func.block, state);
			} break;
		}
	}
}

void VarCompleter::complete(Block& block, State& state) {
	state.descend(block);
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& declaration = (Declaration&)statement;
				Variable& var = state.next_var(declaration.token.str);
				var.type.complete();
				if (!var.type.is_known()) {
					throw Exception("Type could not be determined.", declaration.token);
				}
			} break;
			case Statement::IF: {
				If& if_statement = (If&)statement;
				complete(if_statement.true_block, state);
				complete(if_statement.else_block, state);
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				complete(while_statement.block, state);
			} break;
			default: break;
		}
	}
	state.ascend();
}

