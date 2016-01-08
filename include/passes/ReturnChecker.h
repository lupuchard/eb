#ifndef EBC_RETURNCHECKER_H
#define EBC_RETURNCHECKER_H

#include "State.h"
#include "ast/Module.h"

class ReturnChecker {
public:
	void check(Module& module);

private:
	bool check(Block& block);
	void create_implicit_returns(Block& block);
	void create_drops(Block* block);
	void create_drops(Expr& expr, std::vector<Statement*>& new_statements);
	void create_drop(Block& block, const Token& token, Token& tmp);

	int index = 0;
	std::vector<std::unique_ptr<Token>> phantom_tokens;
};


#endif //EBC_RETURNCHECKER_H
