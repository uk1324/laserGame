#include "Game.hpp"
#include <game/GameSerialization.hpp>
#include <engine/Math/Interpolation.hpp>
#include <imgui/imgui.h>
#include <engine/Input/Input.hpp>

Game::Game() 
	: font(FontRenderer::loadFont("engine/assets/fonts/", "RobotoMono-Regular")) {
	tryLoadLevel("./generated/test");
}

Vec2 snapPositionsOutsideBoundary(Vec2 v) {
	const auto length = v.length();
	const auto maxAllowedLength = Constants::boundary.radius - 0.05f;
	if (length > maxAllowedLength) {
		v *= maxAllowedLength / length;
	}
	return v;
}

void Game::update(GameRenderer& renderer) {
	bool cursorCaptured = false;

	auto cursorPos = Input::cursorPosClipSpace() * renderer.gfx.camera.clipSpaceToWorldSpace();

	const bool cursorExact = false;
	const bool enforceConstrains = true;

	laserGrabTool.update(e.lasers, std::nullopt, cursorPos, cursorCaptured, cursorExact, enforceConstrains);
	for (auto laser : e.lasers) {
		laser->position = snapPositionsOutsideBoundary(laser->position);
	}
	mirrorGrabTool.update(e.mirrors, std::nullopt, cursorPos, cursorCaptured, cursorCaptured, enforceConstrains);
	for (auto mirror : e.mirrors) {
		mirror->center = snapPositionsOutsideBoundary(mirror->center);
	}
	portalGrabTool.update(e.portalPairs, std::nullopt, cursorPos, cursorCaptured, cursorCaptured, enforceConstrains);
	for (auto portalPair : e.portalPairs) {
		for (auto& portal : portalPair->portals) {
			portal.center = snapPositionsOutsideBoundary(portal.center);
		}
	}

	const auto objectsInValidState = areObjectsInValidState();
	s.update(e, objectsInValidState);
	
	renderer.renderClear();
	renderer.render(e, s, false, objectsInValidState);


	bool allTargetsActivated = true;
	for (const auto& target : e.targets) {
		if (!target->activated) {
			allTargetsActivated = false;
		}
	}
	const auto levelComplete = objectsInValidState && allTargetsActivated;

	auto uiCursorPos = Ui::cursorPosUiSpace();

	{
		auto& r = renderer;

		f32 xSize = 0.2f;
		f32 ySize = 0.5f * Ui::xSizeToYSize(r, xSize);
		Vec2 size(xSize, ySize);

		const auto anchor = Vec2(0.5f, -0.5f);
		
		const auto pos = Ui::rectPositionRelativeToCorner(anchor, size, Ui::equalSizeReativeToX(r, 0.01f));

		Ui::updateConstantSpeedT(goToNextLevelButtonActiveT, 0.3f, levelComplete);

		auto hover = Ui::isPointInRectPosSize(pos, size, uiCursorPos);
		// Don't do hover highlighting when the level is not complete so the player doesn't try to press a button that doesn't do anything. Could instead just not have the button there when the level is not complete.
		Ui::updateConstantSpeedT(goToNextLevelButtonHoverT, 0.3f, levelComplete && hover);

		goToNextLevelButtonActiveT = std::clamp(goToNextLevelButtonActiveT, 0.0f, 1.0f);

		const auto color1 = lerp(Color3::WHITE / 15.0f, Color3::WHITE / 10.0f, goToNextLevelButtonHoverT);
		const auto color2 = lerp(1.5f * color1, Color3::WHITE / 3.0f, goToNextLevelButtonActiveT);
		Ui::rectPosSizeFilled(r, pos, size, color1);

		const auto padding = size * 0.1f;
		const auto insideSize = size - padding * 2.0f;
		const auto min = pos - size / 2.0f + padding;
		const auto max = pos + size / 2.0f - padding;
		const auto offset = max.x - Ui::ySizeToXSize(r, insideSize.y / 2.0f * sqrt(2.0f));
		Ui::triFilled(r, 
			Vec2(max.x, pos.y), 
			Vec2(offset, max.y),
			Vec2(offset, min.y),
			color2);
		Ui::rectMinMaxFilled(r,
			Vec2(min.x, min.y + 0.25f * insideSize.y),
			Vec2(offset, max.y - 0.25f * insideSize.y),
			color2);

		{
			static f32 f = 0.0f;
			ImGui::SliderFloat("f", &f, 0.0f, 1.0f);
			const auto of = Vec2(lerp(-1.0f, 1.0f, f), 0.0f);

			const auto min = Vec2(-0.5f) + of;
			const auto max = Vec2(0.5f) + of;
			
			Ui::rectMinMaxFilled(r, min, max, Color3::BLACK);

			/*const auto barWidth = 0.02f;
			addFilledRectMinMax(min, Vec2(min.x + barWidth, max.y), Color3::WHITE / 3.0f);
			addFilledRectMinMax(Vec2(max.x - barWidth, min.y), max, Color3::WHITE / 3.0f);*/
		}
		
	}

	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();

	renderer.gfx.fontRenderer.render(font, renderer.gfx.instancesVbo);
}

void Game::reset() {
	e.reset();
}

bool Game::areObjectsInValidState() {
	std::vector<StereographicSegment> movableObjects;
	for (const auto& mirror : e.mirrors) {
		const auto endpoints = mirror->calculateEndpoints();
		movableObjects.push_back(StereographicSegment(endpoints[0], endpoints[1]));
	}
	for (const auto& portalPair : e.portalPairs) {
		for (const auto& portal : portalPair->portals) {
			const auto endpoints = portal.endpoints();
			movableObjects.push_back(StereographicSegment(endpoints[0], endpoints[1]));
		}
	}
	std::vector<StereographicSegment> staticObjects;
	for (const auto& wall : e.walls) {
		staticObjects.push_back(StereographicSegment(wall->endpoints[0], wall->endpoints[1]));
	}
	for (const auto& door : e.doors) {
		staticObjects.push_back(StereographicSegment(door->endpoints[0], door->endpoints[1]));
	}

	auto collision = [](const StereographicSegment& a, const StereographicSegment& b) {
		return stereographicSegmentVsStereographicSegmentIntersection(a, b).size() > 0;
	};
	for (i32 i = 0; i < i32(movableObjects.size()) - 1; i++) {
		const auto& a = movableObjects[i];
		for (i32 j = i + 1; j < movableObjects.size(); j++) {
			const auto& b = movableObjects[j];
			if (collision(a, b)) {
				return false;
			}
		}
	}
	for (const auto& a : movableObjects) {
		for (const auto& b : staticObjects) {
			if (collision(a, b)) {
				return false;
			}
		}
	}
	for (const auto& a : movableObjects) {
		if (stereographicSegmentVsCircleIntersection(a.line, a.endpoints[0], a.endpoints[1], Constants::boundary).size() > 0) {
			return false;
		}
	}

	return true;
}

bool Game::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevel(e, path);
}