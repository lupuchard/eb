#include "Compiler.h"
#include <iostream>
#include <thread>

Compiler::Compiler(const std::string& filename) {
	initialize(filename);

	std::vector<std::thread> threads;
	threads.resize(files.size());
	size_t i = 0;
	num_threads.store(0);
	for (auto& file : files) {
		while (num_threads.load() >= 4) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		threads[i] = std::thread([&] { compile(file.second); });//&Compiler::compile, &*this, file.second);
		num_threads.fetch_add(1);
	}
	for (auto& thread : threads) {
		thread.join();
	}
}

void Compiler::initialize(const std::string& filename) {
	std::string module_name = filename; // TODO: remove extension, assert is valid identifier
	File& file = files[module_name];
	file.stream.open(filename);
	file.tokens.reset(new Tokenizer(file.stream));
	auto& traits = file.tokens->get_traits();
	for (auto& trait : traits) {
		switch (trait.first) {
			case Trait::INCLUDE: initialize(trait.second); break;
		}
	}
}

void Compiler::compile(File& file) {
	Parser parser;
	file.module = parser.construct(file.tokens->get_tokens());



	// when module identifier is found: possible locations
	// 1. check locally
	// 2. check among imports
	// 3. check among linked libraries
	// 4. check among other directories in compilation
	// 5. check among other files in compilation
}
