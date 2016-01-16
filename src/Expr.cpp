#include "ast/Expr.h"

bool Value::b(const Token& token) const {
	if (!type == Type::Bool) throw ("Expected boolean", token);
	return (bool)integer;
}
bool Value::b() const {
	assert(type == Type::Bool);
	return (bool)integer;
}
uint64_t Value::i(const Token& token) const {
	if (!type.is_int()) throw ("Expected integer", token);
	return integer;
}
uint64_t Value::i() const {
	assert(type.is_int());
	return integer;
}
double Value::f(const Token& token) const {
	if (type == Type::IntLit) return i();
	if (!type.is_float()) throw ("Expected float", token);
	return flt;
}
double Value::f() const {
	if (type == Type::IntLit) return i();
	assert(type.is_float());
	return flt;
}


void insert(Expr* expr, std::vector<std::pair<Tok*, Tok*>>& insertions) {
	if (!insertions.empty()) {
		size_t i = 0;
		Expr new_expr;
		for (auto& utok : *expr) {
			Tok& tok = *utok;
			new_expr.emplace_back();
			new_expr.back().swap(utok);
			if (i < insertions.size() && &tok == insertions[i].first) {
				do {
					new_expr.push_back(std::unique_ptr<Tok>(insertions[i].second));
					i++;
				} while (i < insertions.size() && insertions[i - 1].first == insertions[i].first);
			}
		}
		assert(i == insertions.size());
		*expr = std::move(new_expr);
	}
}

void insert(Expr* expr, std::map<Tok*, Tok*>& insertions) {
	if (!insertions.empty()) {
		Expr new_expr;
		for (auto& utok : *expr) {
			Tok& tok = *utok;
			new_expr.emplace_back();
			new_expr.back().swap(utok);
			auto iter = insertions.find(&tok);
			if (iter != insertions.end()) {
				new_expr.push_back(std::unique_ptr<Tok>(iter->second));
			}
		}
		*expr = std::move(new_expr);
	}
}
