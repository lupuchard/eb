#ifndef EBC_TYPES_H
#define EBC_TYPES_H

#include <string>

enum class Type {
	INVALID, UNKNOWN, VOID,
	NUM,
	SIGNED, I8, I16, I32, I64,
	UNSIGNED, U8, U16, U32, U64,
	FLOAT, F32, F64,
	BOOL,
};

Type get_supertype(Type type);
bool is_type(Type type, Type could_be);
Type merge_types(Type type1, Type type2);
Type complete_type(Type type);

Type parse_type(const std::string& type);
std::string type_to_string(Type type);

#endif //EBC_TYPES_H
