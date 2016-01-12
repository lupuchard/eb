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
	return *iter->second;
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

Variable* Scope::declare(std::string name, Type type) {
	if (variables.count(name)) return nullptr;
	variables[name] = Variable(type);
	return &variables[name];
}


Variable* Scope::get(const std::string& name) {
	auto iter = variables.find(name);
	if (iter == variables.end()) {
		if (parent == nullptr) return nullptr;
		return parent->get(name);
	}
	return &iter->second;
}

State::State(Module& module): module(&module) {
	current_scope = &root_scope;
}

Module& State::get_module() {
	return *module;
}

void State::descend(Block& block) {
	current_scope = &current_scope->to_subscope(block);
}
void State::ascend() {
	current_scope = current_scope->get_parent();
}

Variable* State::declare(std::string name, Type type) {
	return current_scope->declare(name, type);
}
Variable* State::get_var(const std::string& name) const {
	auto var = current_scope->get(name);
	if (var == nullptr) {
		Global* global = module->get_global(name);
		if (global == nullptr) return nullptr;
		return &global->var;
	}
	return var;
}

void State::set_func(Function& func) {
	current_func = &func;
}
Function& State::get_func() const {
	assert(current_func);
	return *current_func;
}

Loop* State::get_loop(int amount) const {
	return current_scope->get_loop(amount);
}
void State::create_loop() {
	return current_scope->create_loop();
}
