#ifndef EBC_EXCEPTION_H
#define EBC_EXCEPTION_H

#include "ast/Token.h"
#include <stdexcept>

struct Except : std::invalid_argument {
	Except(std::string desc, const Token& token);
	Except(std::string desc);
	virtual const char* what() const throw();
	std::string desc;
	Token token;
};

#endif //EBC_EXCEPTION_H
