#ifndef EBC_COMPILER_H
#define EBC_COMPILER_H

#include "Tokenizer.h"
#include "Parser.h"
#include "Builder.h"
#include <string>
#include <fstream>
#include <atomic>

class Compiler {
public:
	Compiler(const std::string& filename);
	void initialize(const std::string& filename);

private:
	struct File {
		std::ifstream stream;
		Module module;
		std::unique_ptr<Tokenizer> tokens;
		std::map<std::string, std::string> imports;
	};
	void compile(File& file);

	std::map<std::string, File> files;

	//shared
	std::atomic_int num_threads;
};


#endif //EBC_COMPILER_H
