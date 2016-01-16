#include "Tokenizer.h"
#include "Util.h"
#include <algorithm>

Tokenizer::Tokenizer(const std::string& str): str(str) {
	do_whitespace();
	this->str.clear();
}

Tokenizer::Tokenizer(std::ifstream& file) {
	std::stringstream buffer;
	buffer << file.rdbuf();
	str = std::move(buffer.str());
	do_whitespace();
	str.clear();
	file.close();
}

const std::vector<Token>& Tokenizer::get_tokens() const {
	return tokens;
}
const std::vector<std::pair<Trait, std::string>>& Tokenizer::get_traits() const {
	return traits;
}

// toss that worthless whitespace
void Tokenizer::do_whitespace() {
	while (true) {
		char c = str[index];
		if (c == '\n') {
			tokens.push_back(Token(Token::END, "\n", line, column));
			line++;
			column = 0;
		} else if (is_valid_ident_beginning(c)) {
			return do_word();
		} else if (isdigit(c) || c == '.') {
			return do_number();
		} else if (!isspace(c)) {
			return do_symbol();
		}
		index++;
		column++;
	}
}


// identifiers
// start with _ or letter, continue as _'s, letters's and number's (non-ascii also allowed)
void Tokenizer::do_word() {
	std::string word;
	word += str[index];
	while (true) {
		column++;
		char c = str[++index];
		if (is_valid_ident(c)) {
			word += c;
		} else {
			auto iter = KEYWORDS.find(word);
			if (iter != KEYWORDS.end()) {
				tokens.push_back(Token(iter->second, word, line, column));
			} else if (connecting) {
				tokens.back().add_str(word);
				connecting = false;
			} else {
				tokens.push_back(Token(Token::IDENT, word, line, column));
			}
			if (isspace(c)) {
				return do_whitespace();
			} else {
				return do_symbol();
			}
		}
	}
}

void Tokenizer::do_number() {
	std::string number;
	number += str[index];
	bool prev_e = false;
	while (true) {
		column++;
		char c = str[++index];
		if (isdigit(c) || isalpha(c) || c == '_' || c == '.' || (prev_e && c == '-')) {
			prev_e = (c == 'e' || c == 'E');
			number += c;
		} else {
			tokens.push_back(Token(Token::FLOAT, number, line, column));
			parse_num(tokens.back());
			if (isspace(c)) {
 				return do_whitespace();
			} else {
				return do_symbol();
			}
		}
	}
}

void Tokenizer::do_symbol() {
	char c = str[index];
	if (!c) return;
	if (c == '/') {
		// Comments!
		column++;
		switch (str[++index]) {
			case 0: return add_symbol("/");
			case '*':
				column += 2;
				while (true) {
					if (!str[++index]) return; // unterminated comment
					if (str[index] == '*' && str[index + 1] == '/') {
						index += 2;
						return do_symbol();
					}
				}
			case '/':
			column = 1;
				while (true) {
					if (!str[index++]) return;
					if (str[index] == '\n') return do_symbol();
				}
			default: add_symbol("/"); break;
		}
	} else if (isspace(c)) {
		return do_whitespace();
	} else if (is_valid_ident_beginning(c)) {
		return do_word();
	} else if (c == '.' && tokens.back().form == Token::IDENT) {
		char d = str[++index];
		if (is_valid_ident_beginning(d)) {
			connecting = true;
			do_word();
		} else {
			add_symbol(str.substr(index - 1, 1));
		}
	} else if (isdigit(c) || (c == '.' && isdigit(str[index + 1]))) {
		return do_number();
	} else if (c == ';') {
		tokens.push_back(Token(Token::END, ";", line, column));
		column++;
		index++;
	} else if (c == '#') {
		column++;
		index++;
		size_t j;
		for (j = 0; !isspace(str[index + j]); j++);
		std::string key = str.substr(index, j);
		auto iter = TRAITS.find(key);
		if (iter == TRAITS.end()) {
			throw Except("'" + key + "' is not a valid trait", BLANK_TOKEN);
		}
		for (index += j; isspace(str[index]); index++);
		for (j = 0; !isspace(str[index + j]); j++);
		traits.push_back(std::make_pair(iter->second, str.substr(index, j)));
		index += j;
		return do_whitespace();
	} else if ((c == '!' || c == '>' || c == '<' || c == '=') && str[index + 1] == '=') {
		add_symbol(str.substr(index, 2));
		column += 2;
		index += 2;
	} else if ((c == '&' || c == '|' || c == '<' || c == '>') && str[index + 1] == c) {
		add_symbol(str.substr(index, 2));
		column += 2;
		index += 2;
	} else {
		column++;
		add_symbol(str.substr(index++, 1));
	}
	do_symbol();
}

void Tokenizer::add_symbol(std::string s) {
	tokens.push_back(Token(Token::SYMBOL, s, line, column));
}


void Tokenizer::parse_num(Token& token) {
	if (token.str() == ".") {
		token.form = Token::SYMBOL;
		return;
	}
	std::string str;
	std::transform(token.str().begin(), token.str().end(), str.begin(), ::tolower);
	if (token.str().size() >= 2 && token.str()[1] == 'x') {
		parse_int(token, str);
	} else if (token.str().find('e') != std::string::npos ||
		       token.str().find('.') != std::string::npos) {
		parse_float(token, str);
	} else {
		parse_int(token, str);
	}
}

void Tokenizer::parse_float(Token& token, const std::string& str) {

	// parse type
	token.suffix = Token::F;
	size_t end_pos = token.str().find('f');
	if (end_pos != std::string::npos) {
		std::string end = token.str().substr(end_pos);
		if      (end == "f32") token.suffix = Token::F32;
		else if (end == "f64") token.suffix = Token::F64;
		else if (end != "f") throw Except("Invalid fp suffix", token);
	}

	// parse value
	std::string body = token.str().substr(0, end_pos);
	if (body == "0in") {
		token.set_flt(std::numeric_limits<double>::infinity());
		return;
	}
	try {
		token.set_flt(std::stod(token.str()));
	} catch (std::invalid_argument ex) {
		throw Except("Invalid float", token);
	} catch (std::out_of_range ex) {
		throw Except("Out of float range", token);
	}
}

void Tokenizer::parse_int(Token& token, const std::string& str) {

	// parse base
	int base = 10;
	size_t offset = 0;
	bool explicit_base = false;
	if (token.str().size() >= 2) {
		offset = 2;
		explicit_base = true;
		switch (token.str()[1]) {
			case 'b': base = 2;  break;
			case 'q': base = 4;  break;
			case 'o': base = 8;  break;
			case 'x': base = 16; break;
			case 'd': base = 10; break;
			default:
				offset = 0;
				explicit_base = false;
		}
	}

	if (!explicit_base && token.str().find('f') != std::string::npos) {
		return parse_float(token, str);
	}

	// parse type
	size_t end_pos = token.str().find('i');
	if (end_pos != std::string::npos) {
		std::string end = token.str().substr(end_pos);
		if      (end == "i8")  token.suffix = Token::I8;
		else if (end == "i16") token.suffix = Token::I16;
		else if (end == "i32") token.suffix = Token::I32;
		else if (end == "i64") token.suffix = Token::I64;
		else if (end == "i")   token.suffix = Token::I;
		else throw Except("Invalid integral suffix", token);
	} else {
		end_pos = token.str().find('u');
		if (end_pos != std::string::npos) {
			std::string end = token.str().substr(end_pos);
			if      (end == "u8")  token.suffix = Token::U8;
			else if (end == "u16") token.suffix = Token::U16;
			else if (end == "u32") token.suffix = Token::U32;
			else if (end == "u64") token.suffix = Token::U64;
			else throw Except("Invalid integral suffix", token);
		} else token.suffix = Token::N;
	}

	// parse value
	std::string body = token.str().substr(offset, end_pos);
	token.form = Token::INT;
	try {
		token.set_int(std::stoull(body, 0, base));
	} catch (std::invalid_argument ex) {
		throw Except("Invalid integer", token);
	} catch (std::out_of_range ex) {
		throw Except("Out of integer range", token);
	}
}
