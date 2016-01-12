#ifndef EBC_VARIABLE_H
#define EBC_VARIABLE_H

#include "ast/Type.h"

namespace llvm { class Value; }

struct Variable {
	Variable() { }
	Variable(Type type): type(type) { }
	Type type = Type::Invalid;
	llvm::Value* llvm = nullptr;
	bool is_param = false;
	bool is_const = false;
};

#endif //EBC_VARIABLE_H
