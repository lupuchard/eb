#include "ast/Type.h"
#include <unordered_map>
#include <iostream>

Type::Type() {
	possible.insert(Prim::UNKNOWN);
}

Type::Type(Prim prim) {
	possible.insert(prim);
}
Type::Type(const std::set<Prim>& prims): possible(prims) {  }

Type Type::invalid() {
	return Type(std::set<Prim> {});
}

void Type::add(Prim prim) {
	auto iter = possible.find(Prim::UNKNOWN);
	if (iter != possible.end()) possible.erase(iter);
	possible.insert(prim);
}
void Type::complete() {
	if (possible.count(Prim::F64) && merge(FLOAT).possible.size() == possible.size()) {
		possible = {Prim::F64};
	} else if (possible.count(Prim::I32) && merge(NUMBER).possible.size() == possible.size()) {
		possible = {Prim::I32};
	}
}

Type Type::merge(const Type& other) const {
	if (other.possible.count(Prim::UNKNOWN)) return *this;
	else if  (possible.count(Prim::UNKNOWN)) return other;
	Type res = Type::invalid();
	std::set_intersection(possible.begin(), possible.end(),
	                      other.possible.begin(), other.possible.end(),
	                      std::inserter(res.possible, res.possible.begin()));
	return res;
}
Type Type::both(const Type& other) const {
	if (other.possible.count(Prim::UNKNOWN)) return other;
	else if  (possible.count(Prim::UNKNOWN)) return *this;
	Type res = Type::invalid();
	std::set_union(possible.begin(), possible.end(), other.possible.begin(), other.possible.end(),
	               std::inserter(res.possible, res.possible.begin()));
	return res;
}


Type Type::parse(const std::string& name) {
	static std::unordered_map<std::string, Prim> types = {
			{"Bool", Prim::BOOL}, {"Int", Prim::I32},
			{"I8", Prim::I8}, {"I16", Prim::I16}, {"I32", Prim::I32}, {"I64", Prim::I64},
			{"U8", Prim::U8}, {"U16", Prim::U16}, {"U32", Prim::U32}, {"U64", Prim::U64},
			{"F32", Prim::F32}, {"F64", Prim::F64},
	};
	auto iter = types.find(name);
	if (iter != types.end()) {
		return Type(iter->second);
	}
	return Type();
}

bool Type::is_known() const {
	return possible.size() == 1 && *possible.begin() != Prim::UNKNOWN;
}
bool Type::is_valid() const {
	return !possible.empty();
}
Prim Type::get() const {
	if (possible.size() > 1) return Prim::UNKNOWN;
	return *possible.begin();
}
bool Type::has(Prim prim) const {
	return possible.count(Prim::UNKNOWN) || possible.count(prim);
}

std::string Type::to_string() const {
	std::string str;
	if (possible.size() > 1) str += "[";
	bool first = true;
	for (Prim prim : possible) {
		if (!first) str += ", ";
		else first = false;
		switch (prim) {
			case Prim::UNKNOWN: str += "Unknown"; break;
			case Prim::I8:      str += "I8";      break;
			case Prim::I16:     str += "I16";     break;
			case Prim::I32:     str += "I32";     break;
			case Prim::I64:     str += "I64";     break;
			case Prim::U8:      str += "U8";      break;
			case Prim::U16:     str += "U16";     break;
			case Prim::U32:     str += "U32";     break;
			case Prim::U64:     str += "U64";     break;
			case Prim::F32:     str += "F32";     break;
			case Prim::F64:     str += "F64";     break;
			case Prim::BOOL:    str += "Bool";    break;
			case Prim::VOID:    str += "Void";    break;
		}
	}
	if (possible.size() > 1) str += "]";
	return str;
}
