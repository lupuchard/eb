#include <Tokenizer.h>
#include "Parser.h"
#include "catch.hpp"

TEST_CASE("function", "[constructor]") {
	std::cout << "Construct function..." << std::endl;
	Tokenizer tokenizer("fn wob(a: I32, b: F64): Bool { return false; }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Function& function = dynamic_cast<Function&>(mod[0]);
	REQUIRE(function.param_types.size() == 2);
	REQUIRE(function.param_names[0]->str() == "a");
	REQUIRE(function.param_types[1] == Type::F64);
	REQUIRE(function.return_type == Type::Bool);
}

TEST_CASE("assignment", "[constructor]") {
	std::cout << "Construct assignment..." << std::endl;
	Tokenizer tokenizer("fn dob() { x = 654 + y;  x += y * 32; }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Block& block = ((Function&)mod[0]).block;
	REQUIRE(block.size() == 2);

	Statement& assignment1 = *block[0];
	REQUIRE(assignment1.token.form == Token::IDENT);
	REQUIRE(assignment1.token.str() == "x");
	REQUIRE(assignment1.token == Token(Token::IDENT, "x"));
	REQUIRE(assignment1.expr.size() == 3);

	Statement& assignment2 = *block[1];
	REQUIRE(assignment2.token == Token(Token::IDENT, "x"));
	Expr& expr = assignment2.expr;
	REQUIRE(expr.size() == 5);
	REQUIRE(expr[0]->form == Tok::VAR);
	REQUIRE(expr[0]->token == &assignment2.token);
	REQUIRE(expr.back()->form == Tok::FUNC);
}

TEST_CASE("declaration", "[constructor]") {
	std::cout << "Construct declaration..." << std::endl;
	Tokenizer tokenizer("fn dob() { x := 3;  y: i32 = x - 4;  z: f64; }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Block& block = ((Function&)mod[0]).block;
	REQUIRE(block.size() == 3);

	Declaration& declaration1 = dynamic_cast<Declaration&>(*block[0]);
	REQUIRE(declaration1.token == Token(Token::IDENT, "x"));
	REQUIRE(declaration1.expr.size() == 1);

	Declaration& declaration2 = dynamic_cast<Declaration&>(*block[1]);
	REQUIRE(declaration2.token == Token(Token::IDENT, "y"));
	REQUIRE(declaration2.expr.size() == 3);

	Declaration& declaration3 = dynamic_cast<Declaration&>(*block[2]);
	REQUIRE(declaration3.token == Token(Token::IDENT, "z"));
	REQUIRE(declaration3.expr.empty());
}

TEST_CASE("expression", "[constructor]") {
	std::cout << "Construct long expression..." << std::endl;
	Tokenizer tokenizer("fn dob() { maths = 5 - 4 * (-3 + 2) / 1; \n lagic = true && (3 <= 4); }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Block& block = ((Function&)mod[0]).block;

	Expr& expr1 = block[0]->expr;
	REQUIRE(expr1.size() == 10);
	REQUIRE(((IntTok&)*expr1[0]).i  == 5);
	REQUIRE(((IntTok&)*expr1[1]).i  == 4);
	REQUIRE(((IntTok&)*expr1[2]).i  == 3);
	REQUIRE(expr1[3]->token->str() == "-");
	REQUIRE(((IntTok&)*expr1[4]).i  == 2);
	REQUIRE(expr1[5]->token->str() == "+");
	REQUIRE(expr1[6]->token->str() == "*");
	REQUIRE(((IntTok&)*expr1[7]).i  == 1);
	REQUIRE(expr1[8]->token->str() == "/");
	REQUIRE(expr1[9]->token->str() == "-");

	Expr& expr2 = block[1]->expr;
	REQUIRE(expr2.size() == 5);
	REQUIRE(((IntTok&)*expr2[0]).i  == 1);
	REQUIRE(((IntTok&)*expr2[1]).i  == 3);
	REQUIRE(((IntTok&)*expr2[2]).i  == 4);
	REQUIRE(expr2[3]->token->str() == "<=");
	REQUIRE(expr2[4]->token->str() == "&&");
}

TEST_CASE("function call", "[constructor]") {
	std::cout << "Construct function calls..." << std::endl;
	Tokenizer tokenizer("fn dob() { x = foo(1, 2 + 3,); x = bar(); x = baz(a, b, c); }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Block& block = ((Function&)mod[0]).block;

	Expr& expr1 = block[0]->expr;
	REQUIRE(expr1[4]->token->str() == "foo");
	REQUIRE(dynamic_cast<FuncTok&>(*expr1[4]).num_params == 2);

	Expr& expr2 = block[1]->expr;
	REQUIRE(dynamic_cast<FuncTok&>(*expr2[0]).num_params == 0);

	Expr& expr3 = block[2]->expr;
	REQUIRE(dynamic_cast<FuncTok&>(*expr3[3]).num_params == 3);
}

TEST_CASE("if", "[constructor]") {
	std::cout << "Construct if..." << std::endl;
	Tokenizer tokenizer("fn t() { if x < y { x += 1 } else if x > y { y += 1 } else { z = 0 } }");
	auto& tokens = tokenizer.get_tokens();
	Parser constructor;
	Module mod;
	constructor.construct(mod, tokens);
	Block& block = ((Function&)mod[0]).block;

	REQUIRE(block.size() == 1);
	If& if_statement = dynamic_cast<If&>(*block[0]);
	REQUIRE(if_statement.expr[2]->token->str() == "<");
	If& elif_statement = dynamic_cast<If&>(*if_statement.else_block[0]);
	REQUIRE(elif_statement.expr[2]->token->str() == ">");

	REQUIRE(  if_statement.true_block[0]->token.str() == "x");
	REQUIRE(elif_statement.true_block[0]->token.str() == "y");
	REQUIRE(elif_statement.else_block[0]->token.str() == "z");
}
