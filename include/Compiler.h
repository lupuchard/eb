#ifndef EBC_COMPILER_H
#define EBC_COMPILER_H

#include "Tokenizer.h"
#include "Constructor.h"
#include "Checker.h"
#include "Builder.h"
#include <string>

class Compiler {
public:

	void compile(const std::string& code, const std::string& out_file);
	void gen_ll(const std::string& code, const std::string& out);

private:
	Constructor constructor;
	Checker     checker;
	Builder     builder;
};


#endif //EBC_COMPILER_H
