#ifndef EBC_SCOPE_H
#define EBC_SCOPE_H

#include "ast/Module.h"
#include "Variable.h"
#include <unordered_map>
#include <map>

namespace llvm {
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
	State(Module& module);

	Module& get_module();

	void descend(Block& block);
	void ascend();

	Variable& declare(std::string name, Type type);
	Variable* get_var(const std::string& name) const;
	Variable& next_var(const std::string& name);

	void set_func(Function& func);
	Function& get_func() const;

	Loop* get_loop(int amount) const;
	void create_loop();

private:
	Module* module;
	Function* current_func = nullptr;
	Scope root_scope;
	Scope* current_scope;
};


#endif //EBC_SCOPE_H
