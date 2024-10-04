#pragma once

#include <gfx2d/Gfx2d.hpp>

struct GameRenderer {
	static GameRenderer make();

	void wall(Vec2 e0, Vec2 e1);
	void renderWalls();

	Gfx2d gfx;
};