#include "GameRenderer.hpp"
#include <engine/Math/Color.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Constants.hpp>

GameRenderer GameRenderer::make() {
	return GameRenderer{
		.gfx = Gfx2d::make()
	};
}

void GameRenderer::wall(Vec2 e0, Vec2 e1) {
	//gfx.line(e0, e1, Constants::wallWidth, Color3::WHITE);
	const auto center = (e0 + e1) / 2.0f;
	const auto angle = (e0 - center).angle();
	gfx.circleArcTriangulated(center, distance(e0, e1) / 2.0f, angle, angle + PI<f32>, Constants::wallWidth, Color3::WHITE);
	gfx.disk(e0, Constants::wallWidth / 2.0f, Color3::RED);
	gfx.disk(e1, Constants::wallWidth / 2.0f, Color3::RED);
}

void GameRenderer::renderWalls() {
	gfx.drawLines();
	gfx.drawDisks();
	gfx.drawFilledTriangles();
}
