#include "ast/Token.h"
#include <vector>
#include <stdexcept>

#ifndef EBC_TOKENIZER_H
#define EBC_TOKENIZER_H

class Tokenizer {
public:
	Tokenizer(const std::string& str);
	const std::vector<Token>& get_tokens() const;

	struct Exception: std::invalid_argument {
		Exception(std::string desc, Token token);
		virtual const char* what() const throw();
		std::string desc;
		Token token;
	};

private:
	void do_whitespace();
	void do_word();
	void do_number();
	void do_symbol();
	void add_symbol(std::string s);
	void interpret_raw();
	void parse_num(Token& token);
	void parse_float(Token& token);
	void parse_int(Token& token);

	std::string str;
	size_t index = 0;
	std::vector<Token> tokens;
	int line = 1;
	int column = 1;
};

#endif //EBC_TOKENIZER_H
