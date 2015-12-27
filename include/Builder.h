#ifndef EBC_BUILDER_H
#define EBC_BUILDER_H

#include "State.h"
#include "ast/Item.h"

#include "llvm/IR/IRBuilder.h"

class Builder {
public:
	std::string build(Module& module, State& state);
	void build(Module& module, State& state, const std::string& out_file);
	void build(Module& module, State& state, llvm::raw_ostream& stream);

private:
	void do_module(Module& module, llvm::Module& llvm_module, State& state);
	bool do_block(llvm::IRBuilder<>& builder, Block& block, State& state);
	llvm::Value* do_expr(llvm::IRBuilder<>& builder, Expr& expr, State& state);
	void do_op(llvm::IRBuilder<>& builder, Op op, Type type,
	           std::vector<llvm::Value*>& value_stack, std::vector<Type>& type_stack);

	llvm::BasicBlock* create_basic_block(std::string name);
	llvm::Type* type_to_llvm(Type type);
	llvm::Constant* default_value(Type type, llvm::Type* llvm_type);

	llvm::LLVMContext* c;
	llvm::Function* llvm_func;
};


#endif //EBC_BUILDER_H
