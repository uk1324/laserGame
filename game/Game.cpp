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

	s.update(e);
	renderer.renderClear();
	renderer.render(e, s, false);
}

void Game::reset() {
	e.reset();
}

bool Game::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevel(e, path);
}