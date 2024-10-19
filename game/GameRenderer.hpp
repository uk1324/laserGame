#pragma once

#include <gfx2d/Gfx2d.hpp>

struct GameRenderer {
	static GameRenderer make();

	void wall(Vec2 e0, Vec2 e1);
	void mirror(Vec2 e0, Vec2 e1);
	void renderWalls();
	void stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color);
	void stereographicSegmentEx(Vec2 e0, Vec2 e1, Vec3 color);

	Gfx2d gfx;
};