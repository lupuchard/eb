#ifndef EBC_CONSTRUCTOR_H
#define EBC_CONSTRUCTOR_H

#include "ast/Module.h"
#include <memory>
#include <ast/Expr.h>

class Parser {
public:
	void construct(Module& module, const std::vector<Token>& tokens);

private:
	void                         do_module(Module& module);
	std::unique_ptr<Item>        do_item();
	std::unique_ptr<Function>    do_function();
	std::unique_ptr<Global>      do_global(bool conzt);
	std::unique_ptr<Import>      do_import(const Token& kw);
	Block                        do_block();
	std::unique_ptr<Statement>   do_statement();
	std::unique_ptr<Declaration> do_declare(const Token& ident);
	std::unique_ptr<Statement>   do_assign( const Token& ident, Op op, const Token* op_token);
	std::unique_ptr<Statement>   do_expr(   const Token& first);
	std::unique_ptr<Statement>   do_return( const Token& kw);
	std::unique_ptr<If>          do_if(     const Token& kw);
	std::unique_ptr<While>       do_while(  const Token& kw);
	std::unique_ptr<Break>       do_break(  const Token& kw);
	void                         do_expr(Expr& expr, const std::string& terminator = ";");

	Type return_type(Op op);
	bool left_assoc(Op op);
	int precedence(Op op);

	void trim();
	void expect(const std::string& str);
	const Token& expect_ident();
	const Token& next(int i = 1);
	const Token& peek(int i = 1);
	void assert_simple_ident(const Token& ident);


	const std::vector<Token>* tokens;
	size_t index = 0;
};


#endif //EBC_CONSTRUCTOR_H
