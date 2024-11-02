#pragma once

#include <gfx2d/Gfx2d.hpp>
#include <game/Constants.hpp>

const auto grabbableCircleRadius = 0.015f;

struct GameRenderer {
	static GameRenderer make();

	void multicoloredSegment(const std::array<Vec2, 2>& endpoints, f32 normalAngle, Vec3 normalColor, Vec3 backColor);
	void wall(Vec2 e0, Vec2 e1, Vec3 color);
	void mirror(Vec2 e0, Vec2 e1);
	void renderWalls();
	void stereographicSegmentOld(Vec2 e0, Vec2 e1, Vec3 color);
	void stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color);
	void stereographicSegment(Vec2 e0, Vec2 e1, Vec4 color, f32 width = Constants::wallWidth);

	Gfx2d gfx;
};

Vec3 movablePartColor(bool isPositionLocked);