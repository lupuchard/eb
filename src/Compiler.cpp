#include <iostream>
#include "Compiler.h"

/*bool file_exists(const std::string& name) {
	if (FILE* file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
}*/

/*std::string exec(const char* cmd) {
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL) {
			result += buffer;
		}
	}
	return result;
}*/

void Compiler::compile(const std::string& code, const std::string& out_file) {
	gen_ll(code, out_file);
	/*if (!file_exists("shim.a")) {
		system("clang -c shim.c");
		system("ar rcs shim.a shim.o");
	}*/
	//std::cout << exec(("llc " + out_file + ".ll").c_str());
	//std::cout << exec(("clang -o " + out_file + " " + out_file + ".s shim.a").c_str());
	//std::cout << system(("./" + out_file).c_str()) << std::endl;
}

void Compiler::gen_ll(const std::string& code, const std::string& out_file) {
	/*Tokenizer tokenizer(code);
	auto tokens = tokenizer.get_tokens();
	Module mod = constructor.construct(tokens);
	State scope;
	checker.check_types(mod, scope);
	checker.complete_types(mod, scope);
	builder.build(mod, scope, out_file + ".ll");*/
}
