#include "StaticEval.h"
#include "ast/Item.h"
#include "Except.h"
#include <cmath>

Value StaticEval::eval(Type& target, Expr& expr) {
	std::vector<Value> stack;
	for (size_t i = 0; i < expr.size(); i++) {
		Tok& tok = *expr[i];
		switch (tok.form) {
			case Tok::VALUE: stack.push_back(((ValueTok&)tok).value); break;
			case Tok::FUNC: {
				FuncTok& ftok = (FuncTok&)tok;
				if (!ftok.token->form == Token::SYMBOL) {
					throw ("Can't yet evaluate functions in constant expr", *tok.token);
				}
				if (ftok.num_args == 1) {
					Value res = eval(ftok.token->str(), stack.back(), *tok.token);
					stack.pop_back();
					stack.push_back(res);
				} else {
					Value a = stack[stack.size() - 2];
					Value b = stack.back();
					Value res = eval(tok.token->str(), a, b, *ftok.token);
					stack.pop_back(); stack.pop_back();
					stack.push_back(res);
				}
			} break;
			default: throw ("Can't yet do that in constant expr", *tok.token);
		}
	}
	if (target.is_int() && stack.back().type.is_int()) {
		stack.back().type = target;
	} else if (target.is_float() && stack.back().type == Type::IntLit) {
		stack.push_back(Value((double)stack.back().i(), target));
	} else if (target != stack.back().type) {
		throw Except("Types don't match: " + target.to_string() + " " +
		             stack.back().type.to_string());
	}
	return stack.back();
}
Value StaticEval::eval(const std::string& op, Value val, const Token& token) {
	switch (op[0]) {
		case '!': return Value(!val.b(token));
		case '-': return val.type.is_float()  ? Value(-val.f(token), val.type):
		                 val.type.is_signed() ? throw Except("Expected signed type", token):
		                                        Value(-val.i(token), val.type);
		case '/': return val.type.is_float()  ? Value(1. / val.f(token), val.type):
		                 val.type.is_signed() ? Value(1 / (int64_t)val.i(token), val.type):
		                                        Value(1 / val.i(token), val.type);
		default: assert(false);
	}
}
Value StaticEval::eval(const std::string& op, Value val1, Value val2, const Token& token) {
	if (val1.type == Type::Bool) {
		return eval(op, val1.b(token), val2.b(token), token);
	} else if (val1.type.is_float() || val2.type.is_float()) {
		return eval(op, val1.f(token), val2.f(token), val1.type.is_float() ? val1.type : val2.type,
		            token);
	} else {
		return eval(op, val1.i(token), val2.i(token), val1.type, token);
	}
}
Value StaticEval::eval(std::string op, bool val1, bool val2, const Token& token) {
	if      (op == "==")              return Value(val1 == val2);
	else if (op == "!=" || op == "^") return Value(val1 != val2);
	else if (op == "&&" || op == "&") return Value(val1 && val2);
	else if (op == "||" || op == "|") return Value(val1 && val2);
	throw Except("Expected boolean operation");
}
Value StaticEval::eval(std::string op, uint64_t val1, uint64_t val2, Type type,const Token& token) {
	if (op.size() == 2) {
		switch (op[0]) {
			case '=': return Value(val1 == val2);
			case '!': return Value(val1 != val2);
			case '>': return op[1] == '='     ? Value(val1 >= val2) :
			                 type.is_signed() ? Value((int64_t)val1 >> (int64_t)val2, type) :
			                                    Value(val1 >> val2, type);
			case '<': return op[1] == '=' ? Value(val1 <= val2) :
			                                Value(val1 << val2, type);
			default: throw Except("Invalid op");
		}
	}
	switch (op[0]) {
		case '+': return Value(val1 + val2, type);
		case '-': return Value(val1 - val2, type);
		case '*': return Value(val1 * val2, type);
		case '/': return type.is_signed() ? Value((int64_t)val1 / (int64_t)val2, type) :
		                                    Value(val1 / val2, type);
		case '%': return type.is_signed() ? Value((int64_t)val1 % (int64_t)val2, type) :
		                                    Value(val1 % val2, type);
		case '&': return Value(val1 & val2, type);
		case '|': return Value(val1 | val2, type);
		case '^': return Value(val1 ^ val2, type);
		case '>': return Value(val1 > val2);
		case '<': return Value(val1 < val2);
		default: throw Except("Invalid operator");
	}
}
Value StaticEval::eval(std::string op, double val1, double val2, Type type, const Token& token) {
	switch (op[0]) {
		case '+': return Value(val1 + val2, type);
		case '-': return Value(val1 - val2, type);
		case '*': return Value(val1 * val2, type);
		case '/': return Value(val1 / val2, type);
		case '%': return Value(fmod(val1, val2), type);
		case '=': return Value(val1 == val2);
		case '!': return Value(val1 != val2);
		case '>': return op.size() == 1 ? Value(val1 >  val2) :
		                 op[1] == '='   ? Value(val1 >= val2) :
		                 throw ("Can't bitshift floats", token);
		case '<': return op.size() == 1 ? Value(val1 <  val2) :
		                 op[1] == '='   ? Value(val1 <= val2) :
		                 throw ("Can't bitshift floats", token);
		default: throw Except("Expected float operation");
	}
}
