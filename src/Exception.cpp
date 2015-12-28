#include "Exception.h"

Exception::Exception(std::string desc, Token token):
		std::invalid_argument(desc), desc(desc), token(token) {
	std::stringstream ss;
	ss << "line " << token.line << ", column " << token.column << " (" << token.str << "): " << desc;
	this->desc = ss.str();
}

const char* Exception::what() const throw() {
	return desc.c_str();
}
