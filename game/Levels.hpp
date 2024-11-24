#pragma once

#include <vector>
#include <optional>
#include <RefOptional.hpp>
#include <Types.hpp>

using LevelIndex = i32;

struct LevelInfo {
	const char* path;
	const char* name;
};

struct Levels {
	Levels();

	std::vector<LevelInfo> levels;
	std::optional<const LevelInfo&> tryGetLevelInfo(LevelIndex index) const;
};