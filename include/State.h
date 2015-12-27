#ifndef EBC_SCOPE_H
#define EBC_SCOPE_H

#include "ast/Item.h"
#include <unordered_map>

namespace llvm {
	class Value;
	class Function;
	class BasicBlock;
}

struct Loop {
	llvm::BasicBlock* start = nullptr;
	llvm::BasicBlock* end   = nullptr;
};

struct Variable {
	Variable() { }
	Variable(Type type): type(type) { }
	Type type = Type::invalid();
	llvm::Value* llvm = nullptr;
	bool is_param = false;
};

class Scope {
public:
	Scope& to_subscope(Block& block);
	Scope* get_parent();

	Variable& declare(std::string name, Type type);
	Variable* get(const std::string& name);
	Variable& next(const std::string& name);

	Loop* get_loop(int amount);
	void create_loop();

private:
	Scope* parent = nullptr;
	std::vector<std::unique_ptr<Scope>> children;
	std::unordered_map<Block*, Scope*> children_map;

	std::unordered_map<std::string, std::pair<int, std::vector<Variable>>> variables;
	std::unique_ptr<Loop> loop;
};

class State {
public:
	State();

	void descend(Block& block);
	void ascend();

	Variable& declare(std::string name, Type type);
	Variable* get_var(const std::string& name);
	Variable& next_var(const std::string& name);

	// returns true if function already exists
	bool declare(std::string name, Function& func);
	Function* get_func(const std::string& name);
	void set_func_llvm(const std::string& name, llvm::Function& func);
	llvm::Function* get_func_llvm(const std::string& name);

	void set_return(Type type);
	Type get_return() const;

	Loop* get_loop(int amount);
	void create_loop();

private:
	std::unordered_map<std::string, Function*> functions;
	std::unordered_map<std::string, llvm::Function*> llvm_functions;
	Type return_type;
	Scope root_scope;
	Scope* current_scope;
};


#endif //EBC_SCOPE_H
