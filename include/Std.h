#ifndef EBC_ARITH_H
#define EBC_ARITH_H

#include "ast/Module.h"

class Std {
public:
	Std();
	void add_operators(Module& module);
	Function* get_cast(Type from, Type to);

private:
	void add_signed(Type type);
	void add_unsigned(Type type);
	void add_float(Type type);
	void add_arith(Type type);
	void add_bitwise(Type type);
	void add_shift(Type type);
	void add_comp(Type type);
	void add_eq(Type type);
	void add_func(std::string name, std::vector<Type> params, Type ret);

	std::vector<std::unique_ptr<Token>> tokens;
	std::vector<std::unique_ptr<Function>> operators;

	std::unordered_map<std::pair<Type, Type>, std::unique_ptr<Function>, pairhash> casts;
};

#endif //EBC_ARITH_H
