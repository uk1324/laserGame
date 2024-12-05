#pragma once

#include <vector>
#include <optional>
#include <filesystem>
#include <string>
#include <RefOptional.hpp>
#include <Types.hpp>

using LevelIndex = i32;

struct Levels;

struct LevelInfo {
	const char* workingDirectoryPath;
	std::string path() const;
	const char* name;
};

struct Levels {
	Levels();

	std::vector<LevelInfo> levels;
	std::optional<const LevelInfo&> tryGetLevelInfo(LevelIndex index) const;
};