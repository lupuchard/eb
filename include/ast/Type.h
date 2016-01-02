#ifndef EBC_TYPES_H
#define EBC_TYPES_H

#include "Util.h"
#include <string>
#include <set>

enum class Prim {
	UNKNOWN, VOID, BOOL,
	I8, I16, I32, I64, U8, U16, U32, U64, F32, F64,
};

class Type {
public:
	Type();
	Type(Prim prim);
	Type(const std::set<Prim>& prims);
	static Type invalid();

	void add(Prim prim);
	void complete();

	Type merge(const Type& other) const;
	Type both(const Type& other) const;

	bool is_known() const;
	bool is_valid() const;
	bool has(Prim prim) const;
	Prim get() const;

	static Type parse(const std::string& name);
	std::string to_string() const;

	inline bool operator==(const Type& rhs) const {
		return possible == rhs.possible;
	}

private:
	std::set<Prim> possible;
};
inline bool operator!=(const Type& lhs, const Type& rhs) {
	return !lhs.operator==(rhs);
}

const Type SIGNED   = Type(std::set<Prim> {Prim::I8, Prim::I16, Prim::I32, Prim::I64});
const Type UNSIGNED = Type(std::set<Prim> {Prim::U8, Prim::U16, Prim::U32, Prim::U64});
const Type FLOAT    = Type(std::set<Prim> {Prim::F32, Prim::F64});
const Type NUMBER   = FLOAT.both(SIGNED.both(UNSIGNED));

#endif //EBC_TYPES_H
