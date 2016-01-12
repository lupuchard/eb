#ifndef EBC_UTIL_H
#define EBC_UTIL_H

#include "util/SimpleGlob.h"
#include <iostream>
#include <sstream>
#include <vector>

inline int exec(const char* cmd) {
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

inline bool is_valid_ident_beginning(char c) {
	return isalpha(c) || c == '_' || !isascii(c);
}
inline bool is_valid_ident(char c) {
	return isalnum(c) || c == '_' || !isascii(c);
}

inline std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

inline std::string combine(const std::vector<std::string>& strs, std::string delim) {
	if (strs.empty()) return "";
	std::string res = strs[0];
	for (size_t i = 1; i < strs.size(); i++) {
		res += delim + strs[i];
	}
	return res;
}

struct pairhash {
	template <typename T, typename U>
	std::size_t operator()(const std::pair<T, U> &x) const {
		size_t hash1 = std::hash<T>()(x.first);
		return hash1 ^ (x.second + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
	}
};

#endif //EBC_UTIL_H
