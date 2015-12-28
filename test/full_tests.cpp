#include "Tokenizer.h"
#include "Constructor.h"
#include "passes/Passer.h"
#include "Builder.h"
#include "Exception.h"
#include "catch.hpp"
#include <iostream>
#include <fstream>

int exec(const char* cmd) {
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return -1;
	char buffer[128];
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != nullptr) {
			std::cout << buffer;
		}
	}
	int status = pclose(pipe);
	return WEXITSTATUS(status);
}

void test(const std::string& filename, int expected_result) {
	std::cout << "Testing " << filename << std::endl;
	std::ifstream file(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();

	Tokenizer tokenizer(buffer.str());
	Constructor constructor;
	Module module;
	try {
		module = constructor.construct(tokenizer.get_tokens());
	} catch (Exception e) {
		FAIL("Constructor failed for '" << filename << "': " << e.what());
	}
	Passer passer;
	State state;
	try {
		passer.pass(module, state);
	} catch (Exception e) {
		FAIL("Passer failed for '" << filename << "': " << e.what());
	}
	Builder builder;
	builder.build(module, state, "out.ll");
	if (exec("llc out.ll")) FAIL("LLVM compilation failed");
	if (exec("clang -o out out.s shim.a")) FAIL("Linking failed");

	REQUIRE(exec("./out") == expected_result);
}

bool file_exists(const std::string& name) {
	if (FILE* file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
}

TEST_CASE("full tests", "[full]") {
	if (!file_exists("shim.a")) {
		system("clang -c shim.c");
		system("ar rcs shim.a shim.o");
	}
	test("test/test_code/simple.eb", 0);
	test("test/test_code/loops.eb", 0);
	test("test/test_code/functional.eb", 0);
	test("test/test_code/overloading.eb", 0);
	test("test/test_code/fib.eb", 0);
}
