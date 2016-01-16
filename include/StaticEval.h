#ifndef EBC_STATICEVAL_H
#define EBC_STATICEVAL_H

#include "ast/Expr.h"

class StaticEval {
public:
	Value eval(Type& target, Expr& expr);

private:
	Value eval(const std::string& op, Value val, const Token& token);
	Value eval(const std::string& op, Value    val1, Value    val2,            const Token& token);
	Value eval(      std::string  op, bool     val1, bool     val2,            const Token& token);
	Value eval(      std::string  op, uint64_t val1, uint64_t val2, Type type, const Token& token);
	Value eval(      std::string  op, double   val1, double   val2, Type type, const Token& token);
};

#endif //EBC_STATICEVAL_H
