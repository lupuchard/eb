#include "passes/Circuiter.h"

void Circuiter::shorten(Module& module) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = *module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				shorten(func.block);
			} break;
		}
	}
}

void Circuiter::shorten(Block& block) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		if (statement.expr != nullptr) {
			shorten(statement.expr);
		}
	}
}

void Circuiter::shorten(std::unique_ptr<Expr>& expr) {
	std::vector<int> has_side_fx_stack;
	std::vector<std::vector<Tok*>> stack;
	std::vector<std::pair<int, int>> range_stack;
	//std::vector<std::pair<std::pair<int, int>, Tok>> new_ifs;
	If* new_if = nullptr;
	const Token* new_if_token = nullptr;
	for (size_t j = 0; j < expr->size(); j++) {
		Tok& tok = expr->at(j);
		switch (tok.form) {
			case Tok::BOOL_LIT: case Tok::INT_LIT: case Tok::FLOAT_LIT: case Tok::VAR:
				has_side_fx_stack.push_back(false);
				stack.push_back(std::vector<Tok*>(1, &tok));
				range_stack.push_back(std::make_pair(j, j + 1));
				break;
			case Tok::IF: {
				has_side_fx_stack.push_back(true);
				stack.push_back(std::vector<Tok*>(1, &tok));
				range_stack.push_back(std::make_pair(j, j + 1));
				If& if_statement = *(If*) tok.something;
				shorten(if_statement.expr);
			} break;
			case Tok::FUNCTION:
				merge(has_side_fx_stack, stack, range_stack, (int)tok.i, true, j);
				stack.back().push_back(&tok);
				break;
			case Tok::OP:
				if ((tok.op == Op::AND || tok.op == Op::OR) && has_side_fx_stack.back()) {
					new_if = new If(tok.token);
					Expr* new_expr = new Expr();
					auto& cond = stack[stack.size() - 2];
					for (size_t i = 0; i < cond.size(); i++) {
						new_expr->push_back(*cond[i]);
					}
					if (tok.op == Op::AND) new_expr->push_back(Tok(tok.token, Op::NOT));
					new_if->expr.reset(new_expr);
					new_if->true_block.emplace_back(new Statement(tok.token, Statement::EXPR));
					new_if->true_block[0]->expr.reset(new Expr());
					new_if->true_block[0]->expr->push_back(Tok(tok.token, tok.op == Op::OR));
					new_expr = new Expr();
					auto& else_expr = stack[stack.size() - 1];
					for (size_t i = 0; i < else_expr.size(); i++) {
						new_expr->push_back(*else_expr[i]);
					}
					new_if->else_block.emplace_back(new Statement(tok.token, Statement::EXPR));
					new_if->else_block[0]->expr.reset(new_expr);
					merge(has_side_fx_stack, stack, range_stack, 2, true, j);
					new_if_token = &tok.token;
					goto end_loop;
					// a || b  -->  if  a { true  } else { b }
					// a && b  -->  if !a { false } else { b }
				} else if (is_binary(tok.op)) {
					merge(has_side_fx_stack, stack, range_stack, 2, false, j);
					stack.back().push_back(&tok);
				} else {
					merge(has_side_fx_stack, stack, range_stack, 1, false, j);
					stack.back().push_back(&tok);
				}
				break;
		}
	}
	end_loop:
	if (new_if == nullptr) return;
	std::unique_ptr<Expr> new_expr(new Expr());
	for (size_t i = 0; i < expr->size(); i++) {
		if (i == range_stack.back().first) {
			Tok tok(*new_if_token, Tok::IF);
			tok.something = (void*)new_if;
			new_expr->push_back(tok);
			i = (size_t)range_stack.back().second - 1;
		} else {
			new_expr->push_back((*expr)[i]);
		}
	}
	/*if (new_ifs.empty()) return;
	std::unique_ptr<Expr> new_expr(new Expr());
	int j = 0;
	for (size_t i = 0; i < expr->size(); i++) {
		if (j < new_ifs.size()) {
			if (i >= new_ifs[j].first.first) {
				new_expr->push_back(new_ifs[j].second);
				if (j + 1 < new_ifs.size()) {
					i = (size_t)std::min(new_ifs[j].first.second - 2, new_ifs[j + 1].first.first);
				} else {
					i = (size_t)new_ifs[j].first.second - 2;
				}
				j++;
			}
		} else {
			new_expr->push_back((*expr)[i]);
		}
	}*/
	expr.swap(new_expr);
	shorten(expr);
}

void Circuiter::merge(std::vector<int>& side_fx_stack, std::vector<std::vector<Tok*>>& stack,
                      std::vector<std::pair<int, int>>& range_stack, int num, bool side_fx,
                      size_t j) {
	std::vector<Tok*> vec;
	std::pair<int, int> range(j, j + 1);
	for (int i = num; i >= 1; i--) {
		auto& to_add = stack[stack.size() - i];
		vec.insert(vec.end(), to_add.begin(), to_add.end());
		side_fx = side_fx || side_fx_stack[side_fx_stack.size() - i];
		range.first  = std::min(range_stack[range_stack.size() - i].first,  range.first);
		range.second = std::max(range_stack[range_stack.size() - i].second, range.second);
	}
	stack.erase(stack.end() - num, stack.end());
	stack.push_back(vec);
	side_fx_stack.erase(side_fx_stack.end() - num, side_fx_stack.end());
	side_fx_stack.push_back(side_fx);
	range_stack.erase(range_stack.end() - num, range_stack.end());
	range_stack.push_back(range);
}
