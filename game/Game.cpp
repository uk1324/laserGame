#include "Game.hpp"
#include <game/GameSerialization.hpp>
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
	for (i32 i = 0; i < movableObjects.size() - 1; i++) {
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