#include "Tokenizer.h"
#include "Constructor.h"
#include "Checker.h"
#include "Builder.h"
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
	} catch (Constructor::Exception e) {
		FAIL("Constructor failed for '" << filename << "': " << e.what());
	}
	Checker checker;
	State state;
	try {
		checker.check_types(module, state);
		checker.complete_types(module, state);
	} catch (Checker::Exception e) {
		FAIL("Checker failed for '" << filename << "': " << e.what());
	}
	Builder builder;
	try {
		builder.build(module, state, "out.ll");
	} catch (Checker::Exception e) {
		FAIL("Builder failed for '" << filename << "': " << e.what());
	}
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
	test("test/test_code/test1.eb", 0);
	test("test/test_code/test2.eb", 0);
}
