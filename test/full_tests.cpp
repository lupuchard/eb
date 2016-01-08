#include "catch.hpp"
#include "Compiler.h"
#include "util/Filesystem.h"

void test(const std::string& filename, int expected_result) {
	std::cout << "Testing " << filename << std::endl;
	std::ifstream file(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();

	Compiler compiler(filename, "../out", "../../out");

	REQUIRE(exec("../../out") == expected_result);
}

bool file_exists(const std::string& name) {
	if (FILE* file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
}

TEST_CASE("full tests", "[full]") {
	bool success = change_directory("test/test_code");
	REQUIRE(success);
	if (!file_exists("shim.a")) {
		system("cp ../../shim.c .");
		system("clang -c shim.c");
		system("ar rcs shim.a shim.o");
	}
	test("simple.eb", 0);
	test("loops.eb", 0);
	test("functional.eb", 0);
	test("overloading.eb", 0);
	test("fib.eb", 0);
	test("short.eb", 0);
	test("import.eb", 8);
}
