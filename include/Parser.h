#ifndef EBC_CONSTRUCTOR_H
#define EBC_CONSTRUCTOR_H

#include "ast/Module.h"
#include <memory>
#include <ast/Expr.h>

class Parser {
public:
	void construct(Module& module, const std::vector<Token>& tokens);

private:
	void                         do_module(Module& module, bool submodule = false);
	std::unique_ptr<Item>        do_item(Module& module);
	std::unique_ptr<Function>    do_function();
	std::unique_ptr<Global>      do_global(bool conzt);
	std::unique_ptr<Import>      do_import(const Token& kw);
	std::unique_ptr<Struct>      do_struct(bool pub, Module& module);
	std::unique_ptr<SubModule>   do_submodule(Module& module, bool extend);
	Block                        do_block();
	std::unique_ptr<Statement>   do_statement();
	std::unique_ptr<Declaration> do_declare(const Token& ident);
	std::unique_ptr<Assignment>  do_assign( const Token& ident, const Token* op_token);
	std::unique_ptr<Statement>   do_expr(   const Token& first);
	std::unique_ptr<Statement>   do_return( const Token& kw);
	std::unique_ptr<If>          do_if(     const Token& kw);
	std::unique_ptr<While>       do_while(  const Token& kw);
	std::unique_ptr<Break>       do_break(  const Token& kw);
	Expr                         do_expr(const std::string& terminator = ";");
	void                         do_expr(Expr& expr, const std::string& terminator = ";");

	void trim();
	void expect(const std::string& str);
	const Token& expect_ident();
	const Token& next(int i = 1);
	const Token& peek(int i = 1);
	void assert_simple_ident(const Token& ident);

	StaticEval eval;
	const std::vector<Token>* tokens;
	size_t index = 0;
};


#endif //EBC_CONSTRUCTOR_H
