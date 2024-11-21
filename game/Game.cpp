#include "Game.hpp"
#include <game/GameSerialization.hpp>
#include <engine/Math/Interpolation.hpp>
#include <imgui/imgui.h>
#include <engine/Input/Input.hpp>

Game::Game() {
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

Game::Result Game::update(GameRenderer& renderer) {
	Result result = ResultNone();

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
	
	bool allTargetsActivated = true;
	for (const auto& target : e.targets) {
		if (!target->activated) {
			allTargetsActivated = false;
		}
	}
	const auto levelComplete = objectsInValidState && allTargetsActivated;

	renderer.renderClear();
	renderer.render(e, s, false, objectsInValidState);

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
		Ui::rectPosSizeFilled(r, pos, size, Vec4(color1, goToNextLevelButtonActiveT));

		if (levelComplete && hover && Input::isMouseButtonDown(MouseButton::LEFT)) {
			result = ResultGoToLevel{};
		}

		const auto padding = size * 0.1f;
		const auto insideSize = size - padding * 2.0f;
		const auto min = pos - size / 2.0f + padding;
		const auto max = pos + size / 2.0f - padding;
		const auto offset = max.x - Ui::ySizeToXSize(r, insideSize.y / 2.0f * sqrt(2.0f));
		const auto color2a = Vec4(color2, goToNextLevelButtonActiveT);
		Ui::triFilled(r, 
			Vec2(max.x, pos.y), 
			Vec2(offset, max.y),
			Vec2(offset, min.y),
			color2a);
		Ui::rectMinMaxFilled(r,
			Vec2(min.x, min.y + 0.25f * insideSize.y),
			Vec2(offset, max.y - 0.25f * insideSize.y),
			color2a);
	}

	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();

	//renderer.gfx.fontRenderer.render(font, renderer.gfx.instancesVbo);

	return result;
}

void Game::reset() {
	goToNextLevelButtonActiveT = 0.0f;
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

	for (const auto& cell : e.lockedCells.cells) {
		const auto cellBounds = e.lockedCells.cellBounds(cell);

		for (const auto& laser : e.lasers) {
			if (cellBounds.containsPoint(laser->position)) {
				return false;
			}
		}

		for (const auto& object : movableObjects) {
			for (const auto& endpoint : object.endpoints) {
				if (cellBounds.containsPoint(endpoint)) {
					return false;
				}
			}
			auto stereographicSegmentVsCircleArcCollision = [&](f32 r) {
				const auto intersections = stereographicSegmentVsCircleIntersection(object.line, object.endpoints[0], object.endpoints[1], Circle(Vec2(0.0f), r));
				for (const auto& intersection : intersections) {
					const auto a = angleToRangeZeroTau(intersection.angle());
					if (a >= cellBounds.minA && a >= cellBounds.maxA) {
						return true;
					}
				}
				return false;
			};

			if (stereographicSegmentVsCircleArcCollision(cellBounds.minR)) {
				return false;
			}
			if (stereographicSegmentVsCircleArcCollision(cellBounds.maxR)) {
				return false;
			}
			const auto v00 = Vec2::fromPolar(cellBounds.minA, cellBounds.minR);
			const auto v10 = Vec2::fromPolar(cellBounds.minA, cellBounds.maxR);
			if (stereographicSegmentVsSegmentCollision(object, v00, v10)) {
				return false;
			}
			const auto v01 = Vec2::fromPolar(cellBounds.maxA, cellBounds.minR);
			const auto v11 = Vec2::fromPolar(cellBounds.maxA, cellBounds.maxR);
			if (stereographicSegmentVsSegmentCollision(object, v11, v01)) {
				return false;
			}
		}

	}

	return true;
}

bool Game::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevelFromFile(e, path);
}

bool Game::tryLoadLevelFromJson(const Json::Value& json) {
	reset();
	return tryLoadGameLevelFromJson(e, json);
}
