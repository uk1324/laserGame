#pragma once

#include <game/Ui.hpp>
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

	f32 goToNextLevelButtonActiveT = 0.0f;
	f32 goToNextLevelButtonHoverT = 0.0f;

	Font font;
};