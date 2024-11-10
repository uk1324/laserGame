#include "Game.hpp"
#include <game/GameSerialization.hpp>

Game::Game() {
	//tryLoadLevel("C:/Users/user/Desktop/game idead/levels/l7");
}

void Game::update(GameRenderer& renderer) {
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