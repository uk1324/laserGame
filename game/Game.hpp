#pragma once

#include <game/GameRenderer.hpp>
#include <game/GrabTools.hpp>
#include <game/GameUpdate.hpp>

struct Game {
	Game();
	void update(GameRenderer& renderer);

	void reset();
	bool areObjectsInValidState();
	bool tryLoadLevel(std::string_view path);

	LaserGrabTool laserGrabTool;
	MirrorGrabTool mirrorGrabTool;
	PortalGrabTool portalGrabTool;

	GameEntities e;
	GameState s;
};