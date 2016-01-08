#ifndef EBC_TOKENIZER_H
#define EBC_TOKENIZER_H

#include "ast/Token.h"
#include "Exception.h"
#include <vector>
#include <stdexcept>
#include <fstream>
#include <unordered_map>

enum class Trait { INCLUDE, OUT_BUILD, OUT_EXEC };

class Tokenizer {
public:
	Tokenizer(const std::string& str);
	Tokenizer(std::ifstream& file);
	const std::vector<Token>& get_tokens() const;
	const std::vector<std::pair<Trait, std::string>>& get_traits() const;

private:
	void do_whitespace();
	void do_word();
	void do_number();
	void do_symbol();
	void add_symbol(std::string s);
	void parse_num(  Token& token);
	void parse_float(Token& token, const std::string&);
	void parse_int(  Token& token, const std::string&);

	std::string str;
	size_t index = 0;

	std::vector<Token> tokens;
	std::vector<std::pair<Trait, std::string>> traits;
	bool connecting = false;
	int line = 1;
	int column = 1;

	const std::unordered_map<std::string, Token::Form> KEYWORDS = {
			{"true", Token::KW_TRUE}, {"false", Token::KW_FALSE},
			{"pub", Token::KW_PUB}, {"fn", Token::KW_FN},
			{"return", Token::KW_RETURN}, {"if", Token::KW_IF}, {"else", Token::KW_ELSE},
			{"while", Token::KW_WHILE}, {"continue", Token::KW_CONTINUE}, {"break", Token::KW_BREAK}
	};
	const std::unordered_map<std::string, Trait> TRAITS = {
			{"include", Trait::INCLUDE},
			{"out_build", Trait::OUT_BUILD}, {"out_exec", Trait::OUT_EXEC}
	};
	const Token BLANK_TOKEN;
};

#endif //EBC_TOKENIZER_H
