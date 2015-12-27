#include <Tokenizer.h>
#include <Constructor.h>
#include <Checker.h>
#include <iostream>
#include "catch.hpp"

/*TEST_CASE("declaration types", "[checker]") {
	Tokenizer tokenizer("fn foo() { x := 3; y := true; z := x; }");
	auto tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	State state;
	Checker checker;
	checker.check_types(block, state);
	checker.complete_types(block, state);
	REQUIRE(state.get_var("x")->type == Type::I32);
	REQUIRE(state.get_var("y")->type == Type::BOOL);
	REQUIRE(state.get_var("z")->type == Type::I32);
}

TEST_CASE("assignment types", "[checker]") {
	Tokenizer tokenizer("fn foo() { x := 3; x += 4f64; }");
	auto tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	State state;
	Checker checker;
	checker.check_types(block, state);
	checker.complete_types(block, state);

	REQUIRE(state.get_var("x")->type == Type::F64);
	Declaration& declaration = (Declaration&)*block[0];
	REQUIRE(declaration.value->at(0).type == Type::F64);
}

TEST_CASE("expression types", "[checker]") {
	Tokenizer tokenizer("fn foo() { x := 5 - 4 * (-3 + 2) / 1.5; }");
	auto tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	State state;
	Checker checker;
	checker.check_types(block, state);
	checker.complete_types(block, state);

	REQUIRE(state.get_var("x")->type == Type::F64);
	Declaration& declaration = (Declaration&)*block[0];
	REQUIRE(declaration.value->at(0).type == Type::F64);
}

TEST_CASE("if types", "[checker]") {
	Tokenizer tokenizer("fn foo() { x := 3; y := 4; if x == 3 && y == 3 { return 1; } }");
	auto tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	State state;
	Checker checker;
	checker.check_types(block, state);
	checker.complete_types(block, state);

	REQUIRE(state.get_var("x")->type == Type::I32);
}*/
