#ifndef EBC_CIRCUITER_H
#define EBC_CIRCUITER_H

#include "ast/Module.h"

class Circuiter {
public:
	void shorten(Module& module);

private:
	void shorten(Block& block);
	void shorten(std::unique_ptr<Expr>& expr);
	void merge(std::vector<int>& side_fx_stack, std::vector<std::vector<Tok*>>& stack,
	           std::vector<std::pair<int, int>>& range_stack, int num, bool side_fx, size_t j);
};


#endif //EBC_CIRCUITER_H
