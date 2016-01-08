#ifndef EBC_MODULE_H
#define EBC_MODULE_H

#include "Item.h"
#include "Tree.h"
#include <map>

class Module {
public:
	// returns true if function already exists
	bool declare(Function& func);
	const std::vector<Function*>& get_functions(int num_parameters, const std::string& name) const;
	const std::vector<Function*>& get_pub_functions() const;

	bool declare(Global& global);
	Global* get_global(const std::string& name);
	const std::vector<Global*>& get_pub_globals() const;

	void push_back(std::unique_ptr<Item> item);
	size_t size() const;
	Item& operator[](size_t index);
	const Item& operator[](size_t index) const;

	bool add_import(const std::vector<std::string>& module_name, Module& module);
	Module* search(const std::vector<std::string>& module_name);

	std::vector<std::string> name;
	std::vector<Function*> external_functions;
	std::vector<Global*> external_globals;

private:
	Tree<Module> imports;
	std::vector<std::unique_ptr<Item>> items;

	// map of (name, num parameters) to a list of functions
	std::map<std::pair<std::string, int>, std::vector<Function*>> functions;
	std::vector<Function*> pub_functions;
	std::unordered_map<std::string, Global*> globals;
	std::vector<Global*> pub_globals;
};

#endif //EBC_MODULE_H
