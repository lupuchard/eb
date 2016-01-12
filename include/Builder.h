#ifndef EBC_BUILDER_H
#define EBC_BUILDER_H

#include "State.h"
#include "ast/Module.h"

#include "llvm/IR/IRBuilder.h"

class Builder {
public:
	void build(Module& module, State& state, const std::string& out_file);

private:
	void do_module(Module& module, llvm::Module& llvm_module, State& state);
	bool do_block(llvm::IRBuilder<>& builder, Block& block, State& state);
	llvm::Value* do_statement(llvm::IRBuilder<>& builder, Statement& statement, State& state);
	llvm::Value* do_expr(llvm::IRBuilder<>& builder, Expr& expr, State& state);
	llvm::Value* do_op(llvm::IRBuilder<>& builder, Function& op, std::vector<llvm::Value*>& args);
	llvm::Value* do_cast(llvm::IRBuilder<>& builder, Function& cast, llvm::Value* arg);

	llvm::BasicBlock* create_basic_block(std::string name);
	llvm::Type* type_to_llvm(Type& type);
	llvm::Constant* default_value(Type& type, llvm::Type* llvm_type);

	llvm::LLVMContext* c;
	llvm::Function* llvm_func;
	std::unordered_map<const Function*, llvm::Constant*> llvm_functions;
};


#endif //EBC_BUILDER_H
