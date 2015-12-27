#ifndef EBC_CONSTRUCTOR_H
#define EBC_CONSTRUCTOR_H

#include "ast/Item.h"
#include <memory>
#include <ast/Expr.h>

class Constructor {
public:
	Module construct(const std::vector<Token>& tokens);

	struct Exception: std::invalid_argument {
		Exception(std::string desc, Token token);
		virtual const char* what() const throw();
		std::string desc;
		Token token;
	};

private:
	Module                       do_module();
	std::unique_ptr<Item>        do_item();
	std::unique_ptr<Function>    do_function();
	Block                        do_block();
	std::unique_ptr<Statement>   do_statement();
	std::unique_ptr<Declaration> do_declare(const Token& ident);
	std::unique_ptr<Assignment>  do_assign( const Token& ident, Op op, const Token* op_token);
	std::unique_ptr<Return>      do_return( const Token& kw);
	std::unique_ptr<If>          do_if(     const Token& kw);
	std::unique_ptr<While>       do_while(  const Token& kw);
	std::unique_ptr<Break>       do_break(  const Token& kw);
	void                         do_expr(Expr& expr, const std::string& terminator = ";");

	Type return_type(Op op);
	bool left_assoc(Op op);
	int precedence(Op op);

	void expect(const std::string& str);
	const Token& expect_ident();
	const Token& next();
	const Token& peek();
	const std::vector<Token>* tokens;
	size_t index = 0;
};


#endif //EBC_CONSTRUCTOR_H
