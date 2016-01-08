#ifndef EBC_TREE_H
#define EBC_TREE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

template<class T>
class Tree {
private:
	struct Node {
		std::unordered_map<std::string, std::unique_ptr<Node>> leaves;
		T* module = nullptr;
	};

	bool add(Node& node, std::vector<const std::string*> module_name, T& module) {
		if (module_name.empty()) {
			if (node.module != nullptr) return false;
			node.module = &module;
			return true;
		}
		auto iter = node.leaves.find(*module_name[0]);
		if (iter == node.leaves.end()) {
			node.leaves[*module_name[0]].reset(new Node());
			iter = node.leaves.find(*module_name[0]);
		}
		module_name.erase(module_name.begin());
		return add(*iter->second, module_name, module);
	}
	T* search(Node& node, std::vector<const std::string*> module_name) {
		if (module_name.empty()) {
			return node.module;
		}
		auto iter = node.leaves.find(*module_name[0]);
		if (iter == node.leaves.end()) return nullptr;
		module_name.erase(module_name.begin());
		return search(*iter->second, module_name);
	}

	Node root;

public:
	bool add(const std::vector<std::string>& module_name, T& module) {
		std::vector<const std::string*> vec;
		for (auto& str : module_name) vec.push_back(&str);
		return add(root, vec, module);
	}
	T* search(const std::vector<std::string>& module_name) {
		std::vector<const std::string*> vec;
		for (auto& str : module_name) vec.push_back(&str);
		return search(root, vec);
	}
};


#endif //EBC_TREE_H
