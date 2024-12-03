#pragma once

#include <game/GameUi.hpp>
#include <game/GrabTools.hpp>
#include <game/Levels.hpp>
#include <game/GameUpdate.hpp>
#include <game/GameAudio.hpp>

struct Game {
	struct ResultNone {};
	struct ResultGoToNextLevel{};
	struct ResultSkipLevel{};
	struct ResultGoToMainMenu {};

	using Result = std::variant<ResultNone, ResultGoToNextLevel, ResultSkipLevel, ResultGoToMainMenu>;

	Game();
	Result update(GameRenderer& renderer, GameAudio& audio);
	std::optional<Result> updateUi(GameRenderer& r, GameAudio& audio, bool levelComplete);

	void onSwitchToGame(GameAudio& audio);
	void onSwitchFromGame(GameAudio& audio);

	void reset();
	bool areObjectsInValidState();
	bool tryLoadLevel(std::string_view path);
	bool tryLoadLevelFromJson(const Json::Value& json);
	std::optional<LevelIndex> currentLevel;
	bool tryLoadGameLevel(const Levels& levels, LevelIndex levelIndex);
	bool tryLoadEditorPreviewLevel(const Json::Value& json);

	bool isEditorPreviewLevelLoaded() const;

	std::optional<f32> timeForWhichAllTasksHaveBeenCompleted = std::numeric_limits<f32>::infinity();

	bool previousFrameLevelComplete = false;
	f32 timeSincePlayedLevelComplete = std::numeric_limits<f32>::infinity();

	bool previousFrameInvalidState = false;
	f32 timeSincePlayedInvalidState = std::numeric_limits<f32>::infinity();

	f32 invalidGameStateAnimationT = 0.0f;

	GameUiButton mainMenuButton;
	GameUiButton skipButton;

	LaserGrabTool laserGrabTool;
	MirrorGrabTool mirrorGrabTool;
	PortalGrabTool portalGrabTool;

	GameEntities e;
	GameState s;

	f32 goToNextLevelButtonActiveT = 0.0f;
	f32 goToNextLevelButtonHoverT = 0.0f;
};