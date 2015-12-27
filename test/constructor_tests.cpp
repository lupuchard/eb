#include <Tokenizer.h>
#include <Constructor.h>
#include <iostream>
#include "catch.hpp"

TEST_CASE("function", "[constructor]") {
	std::cout << "Construct function..." << std::endl;
	Tokenizer tokenizer("fn wob(a: I32, b: F64): Bool { return false; }");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Function& function = dynamic_cast<Function&>(*mod[0]);
	REQUIRE(function.param_types.size() == 2);
	REQUIRE(function.param_names[0]->str == "a");
	REQUIRE(function.param_types[1].get() == Prim::F64);
	REQUIRE(function.return_type.get() == Prim::BOOL);
}

TEST_CASE("assignment", "[constructor]") {
	std::cout << "Construct assignment..." << std::endl;
	Tokenizer tokenizer("fn dob() { x = 654 + y;  x += y * 32; }");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	REQUIRE(block.size() == 2);

	Assignment& assignment1 = dynamic_cast<Assignment&>(*block[0]);
	REQUIRE(assignment1.token.form == Token::IDENT);
	REQUIRE(assignment1.token.str == "x");
	REQUIRE(assignment1.token == Token(Token::IDENT, "x"));
	REQUIRE(assignment1.value->size() == 3);

	Assignment& assignment2 = dynamic_cast<Assignment&>(*block[1]);
	REQUIRE(assignment2.token == Token(Token::IDENT, "x"));
	Expr& expr = *assignment2.value;
	REQUIRE(expr.size() == 5);
	REQUIRE(expr[0].form == Tok::VAR);
	REQUIRE(&expr[0].token == &assignment2.token);
	REQUIRE(expr.back().op == Op::ADD);
}

TEST_CASE("declaration", "[constructor]") {
	std::cout << "Construct declaration..." << std::endl;
	Tokenizer tokenizer("fn dob() { x := 3;  y: i32 = x - 4;  z: f64; }");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	REQUIRE(block.size() == 3);

	Declaration& declaration1 = dynamic_cast<Declaration&>(*block[0]);
	REQUIRE(declaration1.token == Token(Token::IDENT, "x"));
	REQUIRE(declaration1.value->size() == 1);

	Declaration& declaration2 = dynamic_cast<Declaration&>(*block[1]);
	REQUIRE(declaration2.token == Token(Token::IDENT, "y"));
	REQUIRE(declaration2.value->size() == 3);

	Declaration& declaration3 = dynamic_cast<Declaration&>(*block[2]);
	REQUIRE(declaration3.token == Token(Token::IDENT, "z"));
	REQUIRE(declaration3.value.get() == nullptr);
}

TEST_CASE("expression", "[constructor]") {
	std::cout << "Construct long expression..." << std::endl;
	Tokenizer tokenizer("fn dob() { maths = 5 - 4 * (-3 + 2) / 1; \n lagic = true && (3 <= 4); }");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;

	Expr& expr1 = *dynamic_cast<Assignment&>(*block[0]).value;
	REQUIRE(expr1.size() == 10);
	REQUIRE(expr1[0].i == 5);
	REQUIRE(expr1[1].i == 4);
	REQUIRE(expr1[2].i == 3);
	REQUIRE(expr1[3].op == Op::NEG);
	REQUIRE(expr1[4].i == 2);
	REQUIRE(expr1[5].op == Op::ADD);
	REQUIRE(expr1[6].op == Op::MUL);
	REQUIRE(expr1[7].i == 1);
	REQUIRE(expr1[8].op == Op::DIV);
	REQUIRE(expr1[9].op == Op::SUB);

	Expr& expr2 = *dynamic_cast<Assignment&>(*block[1]).value;
	REQUIRE(expr2.size() == 5);
	REQUIRE(expr2[0].b);
	REQUIRE(expr2[1].i == 3);
	REQUIRE(expr2[2].i == 4);
	REQUIRE(expr2[3].op == Op::LEQ);
	REQUIRE(expr2[4].op == Op::AND);
}

TEST_CASE("function call", "[constructor]") {
	std::cout << "Construct function calls..." << std::endl;
	Tokenizer tokenizer("fn dob() { x = foo(1, 2 + 3,); x = bar(); x = baz(a, b, c); }");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;

	Expr& expr1 = *dynamic_cast<Assignment&>(*block[0]).value;
	REQUIRE(expr1[4].form == Tok::FUNCTION);
	REQUIRE(expr1[4].token.str == "foo");
	REQUIRE(expr1[4].i == 2);

	Expr& expr2 = *dynamic_cast<Assignment&>(*block[1]).value;
	REQUIRE(expr2[0].form == Tok::FUNCTION);
	REQUIRE(expr2[0].i == 0);

	Expr& expr3 = *dynamic_cast<Assignment&>(*block[2]).value;
	REQUIRE(expr3[3].form == Tok::FUNCTION);
	REQUIRE(expr3[3].i == 3);
}

TEST_CASE("if", "[constructor]") {
	std::cout << "Construct if..." << std::endl;
	Tokenizer tokenizer("fn t(){if x < y { x += 1; } else if x > y { y += 1; } else { z = 0; }}");
	auto& tokens = tokenizer.get_tokens();
	Constructor constructor;
	Module mod = constructor.construct(tokens);
	Block& block = ((Function&)*mod[0]).block;
	REQUIRE(block.size() == 1);
	If& if_statement = dynamic_cast<If&>(*block[0]);

	REQUIRE(if_statement.conditions.size() == 2);
	REQUIRE((*if_statement.conditions[0])[2].op == Op::LT);
	REQUIRE((*if_statement.conditions[1])[2].op == Op::GT);

	REQUIRE(if_statement.blocks.size() == 3);
	REQUIRE(dynamic_cast<Assignment&>(*if_statement.blocks[0][0]).token.str == "x");
	REQUIRE(dynamic_cast<Assignment&>(*if_statement.blocks[1][0]).token.str == "y");
	REQUIRE(dynamic_cast<Assignment&>(*if_statement.blocks[2][0]).token.str == "z");
}
