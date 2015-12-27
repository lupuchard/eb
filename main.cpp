#include <iostream>
#include <Compiler.h>

using namespace std;

int main(int argc, char** argv) {
	Compiler compiler;
	if (argc < 2) return 1;
	compiler.compile(argv[1], "thang");
	return 0;
}