#ifndef EBC_CIRCUITER_H
#define EBC_CIRCUITER_H

#include "ast/Module.h"

class Circuiter {
public:
	void shorten(Module& module);

private:
	void shorten(Block& block);
	void shorten(Expr& expr);
	void merge(std::vector<int>& side_fx_stack, std::vector<std::vector<std::unique_ptr<Tok>*>>& stack,
	           std::vector<std::pair<int, int>>& range_stack, int num, bool side_fx, size_t j);

	std::vector<std::unique_ptr<Token>> artificial_nots;
};

#endif //EBC_CIRCUITER_H
