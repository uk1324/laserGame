#pragma once

template<typename T>
// Returns true if new element added.
bool addWithoutDuplicating(std::vector<T>& v, const T& i) {
	for (const auto& a : v) {
		if (a == i) {
			return false;
		}
	}
	v.push_back(i);
	return true;
}

template<typename T>
// returns true if removed
bool tryRemove(std::vector<T>& v, const T& i) {
	for (i32 j = 0; j < v.size(); j++) {
		if (v[j] == i) {
			v.erase(v.begin() + j);
			return true;
		}
	}
	return false;
}