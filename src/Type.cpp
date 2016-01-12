#include "ast/Type.h"
#include <unordered_map>
#include <algorithm>
#include <cassert>

Type::Type(Form form): form(form) { }

bool Type::is_number() const {
	return is_int() || is_float();
}
bool Type::is_int() const {
	switch (form) {
		case IntLit: case Int:
		case U8: case U16: case U32: case U64: case UPtr:
		case I8: case I16: case I32: case I64: case IPtr:
			return true;
		default: return false;
	}
}
bool Type::is_signed() const {
	switch (form) {
		case IntLit: case Int:
		case I8: case I16: case I32: case I64: case IPtr:
			return true;
		default: return false;
	}
}
bool Type::is_float() const {
	switch (form) {
		case F32: case F64: case Float: return true;
		default: return false;
	}
}

Type Type::parse(const std::string& name) {
	static std::unordered_map<std::string, Form> types = {
			{"Bool", Bool}, {"Int", Int},
			{"I8", I8}, {"I16", I16}, {"I32", Type::I32}, {"I64", Type::I64}, {"IPtr", IPtr},
			{"U8", U8}, {"U16", U16}, {"U32", Type::U32}, {"U64", Type::U64}, {"UPtr", UPtr},
			{"F32", F32}, {"F64", F64}, {"Float", Float},
	};
	auto iter = types.find(name);
	if (iter != types.end()) {
		return Type(iter->second);
	}
	return Invalid;
}

std::string Type::to_string() const {
	static std::unordered_map<Type, std::string> type_map = {
			{Invalid, "Invalid"}, {Void, "Void"}, {Bool, "Bool"}, {IntLit, "IntLit"}, {Int, "Int"},
			{I8, "I8"}, {I16, "I16"}, {I32, "I32"}, {I64, "I64"}, {IPtr, "IPtr"},
			{U8, "U8"}, {U16, "U16"}, {U32, "U32"}, {U64, "U64"}, {UPtr, "UPtr"},
			{F32, "F32"}, {F64, "F64"}, {Float, "Float"},
	};
	auto iter = type_map.find(form);
	assert(iter != type_map.end());
	return iter->second;
}
bool Type::operator==(const Type& other) const {
	return form == other.form;
}
bool Type::operator<(const Type& other) const {
	return (size_t)form < (size_t)other.form;
}
size_t Type::hash() const {
	return (size_t)form;
}

Type::operator Form() const {
	return form;
}
bool Type::operator==(Form other) const {
	return form == other;
}
