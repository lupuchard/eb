#include "Filesystem.h"
#include <util/Util.h>

#if _WIN32
#include <direct.h>
#include <windows.h>
#define CREATE_DIR(s) CreateDirectory(s)
const char FILE_SEPARATOR = '\\';
#else
#include <unistd.h>
#define CREATE_DIR(s) mkdir(s, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
const char FILE_SEPARATOR = '/';
#endif

std::string concat_paths(const std::string& dir, const std::string& path2) {
	bool dir_s = !dir.empty() && dir.back() == FILE_SEPARATOR;
	bool p2_s  = !path2.empty() && path2[0] == FILE_SEPARATOR;
	if (dir_s != p2_s) return dir + path2;
	if (!dir_s) return dir + std::string(1, FILE_SEPARATOR) + path2;
	return dir + path2.substr(1);
}

std::pair<int, int> get_dash_and_dot(const std::string& filename) {
	int dash = (int)filename.find_last_of(FILE_SEPARATOR);
	if (dash == filename.size() - 1) {
		return std::make_pair(dash, std::string::npos);
	} else if (dash == std::string::npos) {
		dash = -1;
	}
	int start = dash + 1;
	if (filename[start + 1] == '.') start++;
	int dot = (int)filename.find_first_of('.', (size_t)start);
	return std::make_pair(dash, dot);
}
std::string get_file_stem(const std::string& filename) {
	auto points = get_dash_and_dot(filename);
	if (points.second == std::string::npos) return filename.substr((size_t)points.first + 1);
	return filename.substr((size_t)points.first + 1, (size_t)(points.second - points.first - 1));
}
std::string get_extension(const std::string& filename) {
	auto points = get_dash_and_dot(filename);
	if (points.second == std::string::npos) return "";
	return filename.substr((size_t)points.second);
}

std::vector<std::string> glob(const std::string& filename) {
	CSimpleGlob g(SG_GLOB_FULLSORT | SG_GLOB_ONLYFILE);
	g.Add(filename.c_str());
	std::vector<std::string> res;
	for (int n = 0; n < g.FileCount(); ++n) {
		res.push_back(g.File(n));
	}
	return res;
}

void create_directory(const std::string& filename) {
	size_t dash = filename.find_last_of(FILE_SEPARATOR);
	size_t dot  = filename.find_last_of('.');
	std::string dir;
	if (dot > dash) {
		dir = filename.substr(0, dash);
	}
	if (dir[0]     == FILE_SEPARATOR) dir = dir.substr(1);
	if (dir.back() == FILE_SEPARATOR) dir = dir.substr(0, dir.size() - 1);
	auto strs = split(dir, FILE_SEPARATOR);
	std::string str = "";
	for (auto next : strs) {
		str += next + std::string(1, FILE_SEPARATOR);
		CREATE_DIR(str.c_str());
	}
}

bool change_directory(const std::string& directory) {
	return chdir(directory.c_str()) == 0;
}
