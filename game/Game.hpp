#pragma once

#include <game/GameRenderer.hpp>
#include <game/GrabTools.hpp>
#include <game/GameUpdate.hpp>

struct Game {
	Game();
	void update(GameRenderer& renderer);

	void reset();
	bool tryLoadLevel(std::string_view path);

	LaserGrabTool laserGrabTool;

	GameEntities e;
	GameState s;
};