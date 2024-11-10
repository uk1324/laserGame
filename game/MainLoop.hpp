#pragma once

#include <game/Editor.hpp>
#include <game/Game.hpp>
#include <game/LevelSelect.hpp>

struct MainLoop {
	MainLoop();
	void update();

	GameRenderer renderer;
	Editor editor;
	Game game;
	LevelSelect levelSelect;

	enum class State {
		GAME, EDITOR, LEVEL_SELECT
	} state = State::LEVEL_SELECT;
};