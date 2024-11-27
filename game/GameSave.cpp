#include "GameSave.hpp"
#include <JsonFileIo.hpp>
#include <game/Serialization.hpp>
#include <filesystem>
#include <fstream>

const auto savePath = "cached/save.json";
const auto completedName = "completed";

// Storing it in an array like this instead of some bitfiled, because it makes it easier to modify levels without breaking things. The only thing that has to remain is the name.

void GameSave::trySave() {
	// TODO: Maybe move the saving to a worker thread.
	std::filesystem::create_directory("./cached");
	std::ofstream file(savePath);

	auto json = Json::Value::emptyObject();
	if (completedLevels.size() > 0) {
		auto& jsonCompleted = makeArrayAt(json, completedName);
		for (const auto& level : completedLevels) {
			jsonCompleted.push_back(level);
		}
	}

	Json::print(file, json);
	
}

void GameSave::tryLoad() {
	const auto json = tryLoadJsonFromFile(savePath);
	if (!json.has_value()) {
		return;
	}
	try {
		if (const auto& completedJson = tryArrayAt(*json, completedName); completedJson.has_value()) {
			for (const auto& level : *completedJson) {
				completedLevels.insert(level.string());
			}
		}
	} catch (const Json::Value::Exception&) {

	}
}

// Could instead iterate backward. This would give the level after the furthest level that the player has completed to.
std::optional<LevelIndex> GameSave::firstUncompletedLevel(const Levels& levels) const {
	return firstUncompletedLevelAfter(-1, levels);
}

std::optional<LevelIndex> GameSave::firstUncompletedLevelAfter(LevelIndex index, const Levels& levels) const {
	for (LevelIndex i = index + 1; i < levels.levels.size(); i++) {
		const char* name = levels.levels[i].name;
		if (!isCompleted(name)) {
			return i;
		}
	}
	return std::nullopt;
}

void GameSave::markAsCompleted(LevelIndex index, const Levels& levels) {
	const auto info = levels.tryGetLevelInfo(index);
	if (!info.has_value()) {
		return;
	}
	completedLevels.insert(info->name);
}

bool GameSave::isCompleted(const char* levelName) const {
	return completedLevels.contains(levelName);
}

bool GameSave::isCompleted(LevelIndex levelIndex, const Levels& levels) const {
	const auto info = levels.tryGetLevelInfo(levelIndex);
	if (!info.has_value()) {
		return false;
	}
	return isCompleted(info->name);
}
