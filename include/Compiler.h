#ifndef EBC_COMPILER_H
#define EBC_COMPILER_H

#include "Tokenizer.h"
#include "Parser.h"
#include "Builder.h"
#include "Tree.h"
#include <fstream>
#include <atomic>

namespace llvm { class Module; }

class Compiler {
public:
	Compiler(const std::string& filename, std::string out_build = "", std::string out_exec = "");
	void initialize(const std::string& filename);

private:
	struct File {
		enum State { READY, IN_PROGRESS, FINISHED };
		State state = READY;
		std::ifstream stream;
		std::unique_ptr<Tokenizer> tokens;
		Module module;
		std::string out_filename;
	};
	void compile(File& file);
	void resolve(Module& module, State& state);
	void resolve(Module& module, const Block& block, State& state);
	void resolve(Module& module, const Expr& expr,   State& state);
	Module& import(Module& module, const std::vector<std::string>& name, const Token& token);

	std::vector<std::unique_ptr<File>> files;
	Tree<File> file_tree;

	std::string out_build;
	std::string out_exec;
};


#endif //EBC_COMPILER_H
