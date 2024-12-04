#include "SettingsManager.hpp"
#include <JsonFileIo.hpp>
#include <filesystem>
#include <fstream>

const auto settingsPath = "cached/settings.json";

void SettingsManager::tryLoadSettings() {
	const auto json = tryLoadJsonFromFile(settingsPath);
	if (!json.has_value()) {
		settings = SettingsManager::defaultSettings;
		return;
	}
	try {
		settings = fromJson<Settings>(*json);
	} catch (const Json::Value::Exception&) {
		settings = SettingsManager::defaultSettings;
	}
}

void SettingsManager::trySaveSettings() {
	// TODO: Maybe move the saving to a worker thread.
	std::filesystem::create_directory("./cached");
	const auto jsonSettings = toJson(settings);
	std::ofstream file(settingsPath);
	Json::print(file, jsonSettings);
}


Settings SettingsManager::defaultSettings = Settings{
	.audio = {
		//.masterVolume = 0.5f,
		.soundEffectVolume = 0.5f,
		.musicVolume = 0.5f,
	},
	.graphics = {
		.drawBackgrounds = true,
		.fullscreen = false,
	}
};