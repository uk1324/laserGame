#include "GameRenderer.hpp"
#include <engine/Math/Color.hpp>
#include <Overloaded.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Stereographic.hpp>

GameRenderer GameRenderer::make() {
	return GameRenderer{
		.gfx = Gfx2d::make()
	};
}

void GameRenderer::multicoloredSegment(const std::array<Vec2, 2>& endpoints, f32 normalAngle, Vec3 normalColor, Vec3 backColor) {
	const auto normal = Vec2::oriented(normalAngle);
	const auto forward = normal * 0.005f;
	const auto back = -normal * 0.005f;
	stereographicSegment(endpoints[0] + forward, endpoints[1] + forward, normalColor);
	stereographicSegment(endpoints[0] + back, endpoints[1] + back, backColor);
}

void GameRenderer::wall(Vec2 e0, Vec2 e1, Vec3 color) {
	gfx.disk(e0, grabbableCircleRadius, Color3::RED);
	gfx.disk(e1, grabbableCircleRadius, Color3::RED);
	stereographicSegment(e0, e1, color);
}

void GameRenderer::renderWalls() {
	gfx.drawLines();
	gfx.drawFilledTriangles();
	gfx.drawDisks();
}

void GameRenderer::stereographicSegmentOld(Vec2 e0, Vec2 e1, Vec3 color) {
	const auto line = stereographicLineOld(e0, e1);

	const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);

	gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, Constants::wallWidth, color, 1000);
}

void GameRenderer::stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color) { 
	stereographicSegment(e0, e1, Vec4(color, 1.0f));
}

void GameRenderer::stereographicSegment(Vec2 e0, Vec2 e1, Vec4 color, f32 width) {
	const auto stereographicLine = ::stereographicLine(e0, e1);
	if (stereographicLine.type == StereographicLine::Type::LINE) {
		gfx.lineTriangulated(e0, e1, width, color);
	} else {
		const auto& line = stereographicLine.circle;

		const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);
		gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, width, color, 1000);
	}
}

Vec3 movablePartColor(bool isPositionLocked) {
	const auto movableColor = Color3::YELLOW;
	const auto nonMovableColor = Vec3(0.3f);
	return isPositionLocked
		? nonMovableColor
		: movableColor;
}
