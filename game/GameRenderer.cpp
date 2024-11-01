#include "GameRenderer.hpp"
#include <engine/Math/Color.hpp>
#include <Overloaded.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Constants.hpp>
#include <game/Stereographic.hpp>

GameRenderer GameRenderer::make() {
	return GameRenderer{
		.gfx = Gfx2d::make()
	};
}

void GameRenderer::wall(Vec2 e0, Vec2 e1, Vec3 color) {
	gfx.disk(e0, Constants::endpointGrabPointRadius, Color3::RED);
	gfx.disk(e1, Constants::endpointGrabPointRadius, Color3::RED);

	//stereographicSegment(e0, e1, Color3::WHITE);
	stereographicSegment(e0, e1, color);
	//gfx.
	//gfx.circleTriangulated(line.center, line.radius, Constants::wallWidth, Color3::WHITE, 1000);
	// 
	// 
	//e0
	/*const auto center = (e0 + e1) / 2.0f;
	const auto angle = (e0 - center).angle();
	gfx.circleArcTriangulated(center, distance(e0, e1) / 2.0f, angle, angle + PI<f32>, Constants::wallWidth, Color3::WHITE);
	gfx.disk(e0, Constants::wallWidth / 2.0f, Color3::RED);
	gfx.disk(e1, Constants::wallWidth / 2.0f, Color3::RED);*/
}

//void GameRenderer::mirror(Vec2 e0, Vec2 e1){
//}

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
	const auto stereographicLine = ::stereographicLine(e0, e1);
	if (stereographicLine.type == StereographicLine::Type::LINE) {
		gfx.lineTriangulated(e0, e1, Constants::wallWidth, color);
	} else {
		const auto& line = stereographicLine.circle;

		const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);
		gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, Constants::wallWidth, color, 1000);
	}
}

Vec3 movablePartColor(bool isPositionLocked) {
	const auto movableColor = Color3::YELLOW;
	const auto nonMovableColor = Vec3(0.3f);
	return isPositionLocked
		? nonMovableColor
		: movableColor;
}
