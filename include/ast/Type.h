#ifndef EBC_TYPES_H
#define EBC_TYPES_H

#include "Util.h"
#include <string>
#include <set>
#include <memory>

class BaseType;

class Type {
public:
	enum Form {
		Invalid,
		Void,                // for empty returns
		Bool,                // either true or false
		STRUCT, ENUM, TUPLE, // not exist yet
		IntLit,              // unspecified int literal, can implicitly cast to any numeric type
		Int,                 // int_fast32_t
		U8, U16, U32, U64,   // x-bit unsigned int
		I8, I16, I32, I64,   // x-bit signed int
		UPtr, IPtr,          // pointer-sized ints
		F32, F64,            // x-bit floating point
		Float                // long double
	};
	Type(Form form);
	static Type parse(const std::string& str);

	bool is_number() const;
	bool is_int() const;
	bool is_signed() const;
	bool is_float() const;

	std::string to_string() const;

	bool operator==(const Type& other) const;
	inline bool operator!=(const Type& other) const { return !operator==(other); }
	bool operator<(const Type& other) const;

	operator Form() const;
	bool operator==(Form other) const;
	inline bool operator!=(Form other) const { return !operator==(other); }

	size_t hash() const;

private:
	Form form;
};

namespace std  {
	template<> struct hash<Type> {
		size_t operator() (const Type& x) const {
			return x.hash();
		}
	};
}

#endif //EBC_TYPES_H
