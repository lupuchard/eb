#include "Exception.h"

Exception::Exception(std::string desc, const Token& token):
		std::invalid_argument(desc), desc(desc), token(token) {
	std::stringstream ss;
	ss << "line " << token.line << ", column " << token.column;
	ss << " (" << token.str() << "): " << desc;
	this->desc = ss.str();
}
Exception::Exception(std::string desc): std::invalid_argument(desc), desc(desc) { }

const char* Exception::what() const throw() {
	return desc.c_str();
}
