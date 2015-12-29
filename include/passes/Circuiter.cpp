#include "Circuiter.h"

void Circuiter::shorten(Module& module) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				shorten(func.block);
			} break;
		}
	}
}

void Circuiter::shorten(Block& block) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
		}
	}
}
