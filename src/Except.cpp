#include "Except.h"

Except::Except(std::string desc, const Token& token):
		std::invalid_argument(desc), desc(desc), token(token) {
	std::stringstream ss;
	ss << "line " << token.line << ", column " << token.column;
	ss << " (" << token.str() << "): " << desc;
	this->desc = ss.str();
}
Except::Except(std::string desc): std::invalid_argument(desc), desc(desc) { }

const char* Except::what() const throw() {
	return desc.c_str();
}
