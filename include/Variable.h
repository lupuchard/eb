#ifndef EBC_VARIABLE_H
#define EBC_VARIABLE_H

#include "ast/Type.h"

namespace llvm { class Value; }

struct Variable {
	Variable() { }
	Variable(Type type): type(type) { }
	Type type = Type::invalid();
	llvm::Value* llvm = nullptr;
	bool is_param = false;
};

#endif //EBC_VARIABLE_H
