#ifndef EBC_UTIL_H
#define EBC_UTIL_H

#include <set>
#include <algorithm>

template<typename T>
std::set<T> combine(const std::set<T>& a, const std::set<T>& b) {
	std::set<T> res;
	std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(res, res.begin()));
	return res;
}

#endif //EBC_UTIL_H
