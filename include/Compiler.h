#ifndef EBC_COMPILER_H
#define EBC_COMPILER_H

#include "Tokenizer.h"
#include "Parser.h"
#include "Builder.h"
#include "Tree.h"
#include "Std.h"
#include <fstream>
#include <atomic>

namespace llvm { class Module; }

class Compiler {
public:
	Compiler(const std::string& filename, std::string out_build = "", std::string out_exec = "");
	void initialize(const std::string& filename, bool force_recompile = true);

private:
	struct File {
		enum State { READY, IN_PROGRESS, FINISHED };
		State state = READY;
		std::ifstream stream;
		std::unique_ptr<Tokenizer> tokens;
		Module module;
		std::string out_filename;
		std::vector<std::string> includes;
	};
	void compile(File& file);
	void resolve(Module& module, State& state);
	void resolve(Module& module, const Block& block, State& state);
	void resolve(Module& module, const Expr& expr,   State& state);
	Module& import(Module& module, const std::vector<std::string>& name, const Token& token);
	void create_obj_file(File& file);
	void load_obj_file(File& file);

	std::vector<std::unique_ptr<File>> files;
	Tree<File> file_tree;

	std::string out_build;
	std::string out_exec;

	std::vector<Token> extra_tokens;
	std::vector<std::unique_ptr<Function>> extra_functions;
	std::vector<std::unique_ptr<Global>> extra_globals;

	Std std;
};


#endif //EBC_COMPILER_H
