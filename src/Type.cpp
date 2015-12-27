#include "ast/Type.h"
#include <unordered_map>

Type get_supertype(Type type) {
	switch (type) {
		case Type::NUM: case Type::BOOL:
			return Type::UNKNOWN;
		case Type::SIGNED: case Type::UNSIGNED: case Type::FLOAT:
			return Type::NUM;
		case Type::I8: case Type::I16: case Type::I32: case Type::I64:
			return Type::SIGNED;
		case Type::U8: case Type::U16: case Type::U32: case Type::U64:
			return Type::UNSIGNED;
		case Type::F32: case Type::F64:
			return Type::FLOAT;
		default: return Type::INVALID;
	}
}

bool is_type(Type type, Type could_be) {
	while (type != Type::INVALID) {
		if (type == could_be) return true;
		type = get_supertype(type);
	}
	return false;
}

Type merge_types(Type type1, Type type2) {
	Type t = type2;
	while (t != Type::INVALID) {
		if (t == type1) return type2;
		t = get_supertype(t);
	}
	t = type1;
	while (t != Type::INVALID) {
		if (t == type2) return type1;
		t = get_supertype(t);
	}
	return Type::INVALID;
}

Type complete_type(Type type) {
	switch (type) {
		case Type::NUM:      return Type::I32;
		case Type::SIGNED:   return Type::I32;
		case Type::UNSIGNED: return Type::U32;
		case Type::FLOAT:    return Type::F64;
		case Type::UNKNOWN:  return Type::INVALID;
		default: return type;
	}
}

Type parse_type(const std::string& type) {
	static std::unordered_map<std::string, Type> types = {
			{"Bool", Type::BOOL},
			{"I8", Type::I8}, {"I16", Type::I16}, {"I32", Type::I32}, {"I64", Type::I64},
			{"U8", Type::U8}, {"U16", Type::U16}, {"U32", Type::U32}, {"U64", Type::U64},
			{"F32", Type::F32}, {"F64", Type::F64},
	};
	auto iter = types.find(type);
	if (iter == types.end()) return Type::INVALID;
	return iter->second;
}

std::string type_to_string(Type type) {
	switch (type) {
		case Type::INVALID:  return "Invalid";
		case Type::UNKNOWN:  return "Unknown";
		case Type::NUM:      return "Number";
		case Type::SIGNED:   return "Signed Int";
		case Type::I8:       return "I8";
		case Type::I16:      return "I16";
		case Type::I32:      return "I32";
		case Type::I64:      return "I64";
		case Type::UNSIGNED: return "Unsigned Int";
		case Type::U8:       return "U8";
		case Type::U16:      return "U16";
		case Type::U32:      return "U32";
		case Type::U64:      return "U64";
		case Type::FLOAT:    return "Floating Point";
		case Type::F32:      return "F32";
		case Type::F64:      return "F64";
		case Type::BOOL:     return "Bool";
		case Type::VOID:     return "Void";
	}
}
