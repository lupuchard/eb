#include "State.h"

Scope& Scope::to_subscope(Block& block) {
	auto iter = children_map.find(&block);
	if (iter == children_map.end()) {
		children.emplace_back(new Scope());
		children.back()->parent = this;
		children_map[&block] = children.back().get();
		return *children.back();
	}
	Scope& subscope = *iter->second;
	for (auto& var_entry : subscope.variables) {
		if (!var_entry.second.second[0].is_param) {
			var_entry.second.first = -1;
		}
	}
	return subscope;
}

Scope* Scope::get_parent() {
	return parent;
}

Loop* Scope::get_loop(int amount) {
	if (loop == nullptr) {
		if (parent == nullptr) return nullptr;
		return parent->get_loop(amount);
	} else if (amount > 1) {
		if (parent == nullptr) return nullptr;
		return parent->get_loop(amount - 1);
	}
	return loop.get();
}
void Scope::create_loop() {
	loop.reset(new Loop());
}

Variable& Scope::declare(std::string name, Type type) {
	auto iter = variables.find(name);
	if (iter != variables.end()) {
		iter->second.first++;
		iter->second.second.emplace_back(type);
		return iter->second.second[iter->second.first];
	}
	variables[name] = std::pair<int, std::vector<Variable>>(0, {Variable(type)});
	return variables[name].second[0];
}

Variable& Scope::next(const std::string& name) {
	auto& entry = variables[name];
	entry.first++;
	return entry.second[entry.first];
}

Variable* Scope::get(const std::string& name) {
	auto iter = variables.find(name);
	if (iter == variables.end()) {
		if (parent == nullptr) return nullptr;
		return parent->get(name);
	}
	return &iter->second.second[iter->second.first];
}

State::State() {
	current_scope = &root_scope;
}

void State::descend(Block& block) {
	current_scope = &current_scope->to_subscope(block);
}
void State::ascend() {
	current_scope = current_scope->get_parent();
}


Variable& State::declare(std::string name, Type type) {
	return current_scope->declare(name, type);
}
Variable* State::get_var(const std::string& name) {
	return current_scope->get(name);
}
Variable& State::next_var(const std::string& name) {
	return current_scope->next(name);
}

bool State::declare(std::string name, Function& func) {
	auto iter = functions.find(name);
	if (iter != functions.end()) return true;
	functions[name] = &func;
	return false;
}
Function* State::get_func(const std::string& name) {
	auto iter = functions.find(name);
	if (iter == functions.end()) return nullptr;
	return iter->second;
}
void State::set_func_llvm(const std::string& name, llvm::Function& get_func_llvm) {
	llvm_functions[name] = &get_func_llvm;
}
llvm::Function* State::get_func_llvm(const std::string& name) {
	return llvm_functions[name];
}

void State::set_return(Type type) {
	return_type = type;
}
Type State::get_return() const {
	return return_type;
}

Loop* State::get_loop(int amount) {
	return current_scope->get_loop(amount);
}
void State::create_loop() {
	return current_scope->create_loop();
}
