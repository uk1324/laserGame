#include "Game.hpp"
#include <game/GameSerialization.hpp>
#include <engine/Input/Input.hpp>

Game::Game() {
	tryLoadLevel("C:/Users/user/Desktop/game idead/gametest");
}

void Game::update(GameRenderer& renderer) {
	bool cursorCaptured = false;

	auto cursorPos = Input::cursorPosClipSpace() * renderer.gfx.camera.clipSpaceToWorldSpace();

	const bool cursorExact = false;
	const bool enforceConstrains = true;

	laserGrabTool.update(e.lasers, std::nullopt, cursorPos, cursorCaptured, cursorExact, enforceConstrains);

	s.update(e);
	renderer.renderClear();
	renderer.render(e, s);
}

void Game::reset() {
	e.reset();
}

bool Game::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevel(e, path);
}