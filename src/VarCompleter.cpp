#include "passes/VarCompleter.h"
#include "Exception.h"

void VarCompleter::complete(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				state.descend(func.block);
				complete(func.block, state);
				state.ascend();
			} break;
		}
	}
}

void VarCompleter::complete(Block& block, State& state) {
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
				for (Block& if_block : if_statement.blocks) {
					state.descend(if_block);
					complete(if_block, state);
					state.ascend();
				}
			} break;
			case Statement::WHILE: {
				While& while_statement = (While&)statement;
				state.descend(while_statement.block);
				complete(while_statement.block, state);
				state.ascend();
			} break;
			default: break;
		}
	}
}

