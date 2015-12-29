#ifndef EBC_SCOPE_H
#define EBC_SCOPE_H

#include "ast/Module.h"
#include "Variable.h"
#include <unordered_map>
#include <map>

namespace llvm {
	class Function;
	class BasicBlock;
}

struct Loop {
	llvm::BasicBlock* start = nullptr;
	llvm::BasicBlock* end   = nullptr;
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

	bool declare(Global& global);
	Variable& declare(std::string name, Type type);
	Variable* get_var(const std::string& name) const;
	Variable& next_var(const std::string& name);

	// returns true if function already exists
	bool declare(Function& func);
	const std::vector<Function*>& get_functions(int num_parameters, const std::string& name) const;
	void set_func_llvm(const Function& func, llvm::Function& llvm_func);
	llvm::Function* get_func_llvm(const Function& func);

	void set_return(Type type);
	Type get_return() const;

	Loop* get_loop(int amount) const;
	void create_loop();

private:
	// map of (name, num parameters) to a list of functions
	std::map<std::pair<std::string, int>, std::vector<Function*>> functions;
	std::unordered_map<const Function*, llvm::Function*> llvm_functions;
	Type return_type;

	std::unordered_map<std::string, Variable*> globals;

	Scope root_scope;
	Scope* current_scope;
};


#endif //EBC_SCOPE_H
