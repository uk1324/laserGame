#pragma once

#include <game/SettingsData.hpp>

struct SettingsManager {
	Settings settings;

	static Settings defaultSettings;

	void tryLoadSettings();
	void trySaveSettings();
};