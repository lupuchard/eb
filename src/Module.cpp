#include "ast/Module.h"

bool Module::declare(Function& func) {
	if (func.pub) pub_functions.push_back(&func);
	auto key = std::make_pair(func.token.str(), func.param_names.size());
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
const std::vector<Function*>& Module::get_functions(int num_params, const std::string& name) const {
	auto key = std::make_pair(name, num_params);
	auto iter = functions.find(key);
	if (iter == functions.end()) {
		static std::vector<Function*> empty;
		return empty;
	}
	return iter->second;
}
const std::vector<Function*>& Module::get_pub_functions() const {
	return pub_functions;
}

bool Module::declare(Global& global) {
	if (globals.count(global.token.str())) return true;
	if (global.pub) pub_globals.push_back(&global);
	globals[global.token.str()] = &global;
	return false;
}
Global* Module::get_global(const std::string& name) {
	auto iter = globals.find(name);
	if (iter == globals.end()) return nullptr;
	return iter->second;
}
const std::vector<Global*>& Module::get_pub_globals() const {
	return pub_globals;
}

bool Module::declare(Struct& strukt) {
	if (structs.count(strukt.token.str())) return true;
	if (strukt.pub) pub_structs.push_back(&strukt);
	structs[strukt.token.str()] = &strukt;
	return false;
}
Struct* Module::get_struct(const std::string& name) {
	auto iter = structs.find(name);
	if (iter == structs.end()) return nullptr;
	return iter->second;
}
const std::vector<Struct*>& Module::get_pub_structs() const {
	return pub_structs;
}

Module* Module::create_submodule(const std::string& name) {
	submodules.push_back(std::unique_ptr<Module>(new Module()));
	std::vector<std::string> vec(1, name);
	bool success = imports.add(vec, *submodules.back());
	if (!success) return nullptr;
	return &*submodules.back();
}

void Module::push_back(std::unique_ptr<Item> item) {
	return items.push_back(std::move(item));
}
size_t Module::size() const {
	return items.size();
}
Item& Module::operator[](size_t index) {
	return *items[index];
}
const Item& Module::operator[](size_t index) const {
	return *items[index];
}

bool Module::add_import(const std::vector<std::string>& module_name, Module& module) {
	return imports.add(module_name, module);
}
Module* Module::search(const std::vector<std::string>& module_name) {
	return imports.search(module_name);
}

