#include "Type.h"
#include <string>
#include <sstream>
#include <cstdint>

#ifndef EBC_TOKEN_H
#define EBC_TOKEN_H

struct Token {
	enum Form {
		NONE, INVALID,
		FLOAT, INT,
		IDENT, SYMBOL, SPECIAL,
		KW_FN, KW_TRUE, KW_FALSE, KW_RETURN, KW_IF, KW_ELSE, KW_WHILE, KW_BREAK, KW_CONTINUE,
	};

	Token() { }
	Token(Form form, std::string str, int line = -1, int column = 0):
		form(form), str(str), line(line), column(column) { }
	Token(Type type): type(type) { }

	Form form = NONE;
	std::string str;
	union {
		uint64_t i;
		double f;
	};
	Type type = Type::invalid();

	int line, column;
};

inline bool operator==(const Token& lhs, const Token& rhs) {
	return lhs.form == rhs.form && lhs.str == rhs.str;
}
inline bool operator!=(const Token& lhs, const Token& rhs) {
	return !operator==(lhs, rhs);
}
inline std::ostream& operator<<(std::ostream& os, const Token& token) {
	std::stringstream ss;
	ss << "(" << (int)token.form << ", " << token.str << ")";
	os << ss.str();
	return os;
}

#endif //EBC_TOKEN_H
