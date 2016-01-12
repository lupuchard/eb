#include "passes/LoopChecker.h"
#include "Exception.h"

void LoopChecker::check(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				check(func.block, state);
			} break;
			default: break;
		}
	}
}

void LoopChecker::check(Block& block, State& state) {
	state.descend(block);
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		const Token& token = statement.token;
		switch (statement.form) {
			case Statement::WHILE: {
				state.descend(((While&)statement).block);
				state.create_loop();
				state.ascend();
			} break;
			case Statement::CONTINUE:
				if (state.get_loop(1) == nullptr) {
					throw Exception("No loop to continue", statement.token);
				} break;
			case Statement::BREAK:
				if (state.get_loop(((Break&)statement).amount) == nullptr) {
					throw Exception("No loop to break out of", statement.token);
				} break;
			default: break;
		}
		for (Block* inner_block : statement.blocks()) {
			check(*inner_block, state);
		}
	}
	state.ascend();
}
