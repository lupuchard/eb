#include "Std.h"

Std::Std() {
	add_signed(Type::IntLit);
	add_signed(Type::Int);
	add_signed(Type::I8);
	add_signed(Type::I16);
	add_signed(Type::I32);
	add_signed(Type::I64);
	add_signed(Type::IPtr);

	add_unsigned(Type::U8);
	add_unsigned(Type::U16);
	add_unsigned(Type::U32);
	add_unsigned(Type::U64);
	add_unsigned(Type::UPtr);

	add_float(Type::F32);
	add_float(Type::F64);
	add_float(Type::Float);

	add_bitwise(Type::Bool);
	add_eq(Type::Bool);
	add_func("!", {Type::Bool}, Type::Bool);
	add_func("&&", {Type::Bool, Type::Bool}, Type::Bool);
	add_func("||",  {Type::Bool, Type::Bool}, Type::Bool);
}

void Std::add_signed(Type type) {
	add_arith(type);
	add_bitwise(type);
	add_shift(type);
	add_comp(type);
	add_eq(type);
	add_func("-", {type}, type);
}

void Std::add_unsigned(Type type) {
	add_arith(type);
	add_bitwise(type);
	add_shift(type);
	add_comp(type);
	add_eq(type);
}
void Std::add_float(Type type) {
	add_arith(type);
	add_comp(type);
	add_eq(type);
}
void Std::add_arith(Type type) {
	add_func("+", {type, type}, type);
	add_func("-", {type, type}, type);
	add_func("*", {type, type}, type);
	add_func("/", {type, type}, type);
	add_func("%", {type, type}, type);
	add_func("/", {type}, type);
}
void Std::add_bitwise(Type type) {
	add_func("&", {type, type}, type);
	add_func("|", {type, type}, type);
	add_func("^", {type, type}, type);
}
void Std::add_shift(Type type) {
	add_func(">>", {type, type}, type);
	add_func("<<", {type, type}, type);
}
void Std::add_comp(Type type) {
	add_func(">",  {type, type}, Type::Bool);
	add_func("<",  {type, type}, Type::Bool);
	add_func(">=", {type, type}, Type::Bool);
	add_func("<=", {type, type}, Type::Bool);
}
void Std::add_eq(Type type) {
	add_func("==",  {type, type}, Type::Bool);
	add_func("!=", {type, type}, Type::Bool);
}

void Std::add_func(std::string name, std::vector<Type> params, Type ret) {
	Token* token = new Token(Token::IDENT, name);
	tokens.push_back(std::unique_ptr<Token>(token));
	Function* func = new Function(*token);
	func->param_names.resize(params.size());
	func->param_types = params;
	func->return_type = ret;
	func->form = Function::OP;
	operators.push_back(std::unique_ptr<Function>(func));
}

void Std::add_operators(Module& module) {
	for (size_t i = 0; i < operators.size(); i++) {
		module.declare(*operators[i]);
	}
}

Function* Std::get_cast(Type from, Type to) {
	auto iter = casts.find(std::make_pair(from, to));
	if (iter == casts.end()) {
		// assert is valid implicit primitive cast
		if (to == Type::IntLit) return nullptr;
		if (!((from == Type::IntLit && to.is_number()) || (from.is_int() && to.is_int()))) {
			return nullptr;
		}
		Token* token = new Token(Token::IDENT, from.to_string() + "->" + to.to_string());
		tokens.push_back(std::unique_ptr<Token>(token));
		Function* func = new Function(*token);
		func->param_names.resize(1);
		func->param_types.push_back(from);
		func->return_type = to;
		func->form = Function::CAST;
		casts[std::make_pair(from, to)] = std::unique_ptr<Function>(func);
		return func;
	}
	return &*iter->second;
}
