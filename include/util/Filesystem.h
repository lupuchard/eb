#ifndef EBC_FILESYSTEM_H
#define EBC_FILESYSTEM_H

#include "SimpleGlob.h"
#include <string>
#include <vector>

std::string concat_paths(const std::string& dir, const std::string& path2);

std::string get_file_stem(const std::string& filename);
std::string get_extension(const std::string& filename);

std::vector<std::string> glob(const std::string& filename);

void create_directory(const std::string& filename);
bool change_directory(const std::string& directory);

#endif //EBC_FILESYSTEM_H
