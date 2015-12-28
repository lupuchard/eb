#ifndef EBC_EXCEPTION_H
#define EBC_EXCEPTION_H

#include "ast/Token.h"
#include <stdexcept>

struct Exception: std::invalid_argument {
	Exception(std::string desc, Token token);
	virtual const char* what() const throw();
	std::string desc;
	Token token;
};

#endif //EBC_EXCEPTION_H
