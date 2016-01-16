#include "ast/Type.h"
#include <unordered_map>
#include <algorithm>
#include <ast/Item.h>

Type::Type(Form form): form(form) { }
Type::Type(const Token& token): form(Unresolved), token(&token) { }
Type::Type(Struct& strukt): form(STRUCT), strukt(&strukt) { }
Type::Type(Token::Suffix suffix) {
	static std::unordered_map<int, Form> types = {
			{Token::N, IntLit}, {Token::I, Int}, {Token::IPtr, IPtr}, {Token::UPtr, UPtr},
			{Token::I8, I8}, {Token::I16, I16}, {Token::I32, Type::I32}, {Token::I64, Type::I64},
			{Token::U8, U8}, {Token::U16, U16}, {Token::U32, Type::U32}, {Token::U64, Type::U64},
			{Token::F32, F32}, {Token::F64, F64}, {Token::F, Float},
	};
	assert(types.count(suffix));
	form = types[suffix];
}

int Type::size() const {
	switch (form) {
		case IntLit:                  return sizeof(uintmax_t);
		case Int:                     return sizeof(int_fast32_t);
		case U8:  case I8: case Bool: return 1;
		case U16: case I16:           return 2;
		case U32: case I32: case F32: return 4;
		case U64: case I64: case F64: return 8;
		case UPtr: case IPtr:         return sizeof(uintptr_t);
		case Float:                   return sizeof(long double);
		default: assert(false); break;
	}
}

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
bool Type::is_struct() const  {
	return form == STRUCT;
}

Type Type::parse(const Token& token) {
	Type type(token);
	if (token.ident().size() > 1) return type;
	static std::unordered_map<std::string, Form> types = {
			{"Bool", Bool}, {"Int", Int},
			{"I8", I8}, {"I16", I16}, {"I32", Type::I32}, {"I64", Type::I64}, {"IPtr", IPtr},
			{"U8", U8}, {"U16", U16}, {"U32", Type::U32}, {"U64", Type::U64}, {"UPtr", UPtr},
			{"F32", F32}, {"F64", F64}, {"Float", Float},
	};
	auto iter = types.find(token.str());
	if (iter != types.end()) {
		type.form = iter->second;
	}
	return type;
}

std::string Type::to_string() const {
	if (form == Type::STRUCT) {
		return strukt->unique_name;
	}
	static std::unordered_map<Type, std::string> type_map = {
			{Invalid, "Invalid"}, {Void, "Void"}, {Bool, "Bool"}, {IntLit, "IntLit"}, {Int, "Int"},
			{I8, "I8"}, {I16, "I16"}, {I32, "I32"}, {I64, "I64"}, {IPtr, "IPtr"},
			{U8, "U8"}, {U16, "U16"}, {U32, "U32"}, {U64, "U64"}, {UPtr, "UPtr"},
			{F32, "F32"}, {F64, "F64"}, {Float, "Float"},
	};
	auto iter = type_map.find(form);
	if (iter == type_map.end()) {
		std::stringstream ss;
		ss << "(error:" << form << ")";
		return ss.str();
	}
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
