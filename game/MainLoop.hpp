#pragma once

#include <game/Editor.hpp>
#include <game/Game.hpp>
#include <game/GameRenderer.hpp>

struct MainLoop {
	MainLoop();
	void update();

	GameRenderer renderer;
	Editor editor;
	Game game;

	enum class State {
		GAME, EDITOR, LEVEL_SELECT
	} state = State::GAME;
};