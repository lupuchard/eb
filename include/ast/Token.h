#include "Type.h"
#include <string>
#include <sstream>
#include <cstdint>
#include <cassert>

#ifndef EBC_TOKEN_H
#define EBC_TOKEN_H

struct Token {
	enum Form {
		NONE, INVALID, END, FLOAT, INT, KW_TRUE, KW_FALSE, IDENT, SYMBOL,
		KW_PUB, KW_FN, KW_RETURN, KW_IF, KW_ELSE, KW_WHILE, KW_BREAK, KW_CONTINUE,
	};

	Token() { }
	Token(Form form, std::string str, int line = -1, int column = 0):
		form(form), line(line), column(column) {
		str_list.push_back(str);
	}

	int line, column;
	Form form = NONE;
	Type type = Type::Invalid;

	inline const std::string& str() const {
		assert(!str_list.empty());
		return str_list[0];
	}
	inline const std::vector<std::string>& ident() const {
		return str_list;
	}

	inline uint64_t i() const {
		assert(form == INT);
		return _i;
	}
	inline double f() const {
		assert(form == FLOAT);
		return _f;
	}

	inline void set_int(uint64_t i) {
		assert(form == INT);
		_i = i;
	}
	inline void set_flt(double f) {
		assert(form == FLOAT);
		_f = f;
	}
	inline void add_str(std::string str) { str_list.push_back(str); }

private:
	std::vector<std::string> str_list;
	union {
		uint64_t _i;
		double _f;
	};
};

inline bool operator==(const Token& lhs, const Token& rhs) {
	return lhs.form == rhs.form && lhs.str() == rhs.str();
}
inline bool operator!=(const Token& lhs, const Token& rhs) {
	return !operator==(lhs, rhs);
}
inline std::ostream& operator<<(std::ostream& os, const Token& token) {
	std::stringstream ss;
	ss << "(" << (int)token.form << ", " << token.str() << ")";
	os << ss.str();
	return os;
}

#endif //EBC_TOKEN_H
