#include <iostream>
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
Variable* State::get_var(const std::string& name) const {
	return current_scope->get(name);
}
Variable& State::next_var(const std::string& name) {
	return current_scope->next(name);
}

bool State::declare(std::string name, Function& func) {
	auto key = std::make_pair(name, func.param_names.size());
	auto iter = functions.find(key);
	if (iter == functions.end()) {
		std::vector<Function*> vec;
		vec.push_back(&func);
		functions[key] = vec;
	} else {
		for (Function* f : iter->second) {
			if (f->param_types == func.param_types) {
				return true;
			}
		}
		func.index = (int)iter->second.size();
		iter->second.push_back(&func);
	}
	return false;
}
const std::vector<Function*>& State::get_functions(int num_params, const std::string& name) const {
	auto key = std::make_pair(name, num_params);
	auto iter = functions.find(key);
	if (iter == functions.end()) {
		static std::vector<Function*> empty;
		return empty;
	}
	return iter->second;
}

void State::set_func_llvm(const Function& func, llvm::Function& llvm_func) {
	llvm_functions[&func] = &llvm_func;
}
llvm::Function* State::get_func_llvm(const Function& name) {
	return llvm_functions[&name];
}

void State::set_return(Type type) {
	return_type = type;
}
Type State::get_return() const {
	return return_type;
}

Loop* State::get_loop(int amount) const {
	return current_scope->get_loop(amount);
}
void State::create_loop() {
	return current_scope->create_loop();
}
