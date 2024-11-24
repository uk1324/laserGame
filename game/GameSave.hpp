#pragma once

#include <game/Levels.hpp>
#include <unordered_set>
#include <string>

struct GameSave {
	void trySave();
	void tryLoad();

	std::unordered_set<std::string> completedLevels;

	std::optional<LevelIndex> firstUncompletedLevel(const Levels& levels) const;
	std::optional<LevelIndex> firstUncompletedLevelAfter(LevelIndex index, const Levels& levels) const;

	void markAsCompleted(LevelIndex index, const Levels& levels);

	bool isCompleted(const char* levelName) const;
	bool isCompleted(LevelIndex levelIndex, const Levels& levels) const;
};