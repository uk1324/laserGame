#include "MainLoop.hpp"
#include <engine/Input/Input.hpp>
#include <engine/gfx/ShaderManager.hpp>

MainLoop::MainLoop()
	: renderer(GameRenderer::make()) {
}

void MainLoop::update() {
	if (Input::isKeyDown(KeyCode::TAB)) {
		switch (state) {
			using enum State;
		case GAME:
			state = State::EDITOR;
			break;

		case EDITOR:
			state = State::GAME;
			break;

		default:
			break;
		}
	}

	switch (state) {
		using enum State;
	case GAME:
		game.update(renderer);
		break;

	case EDITOR:
		editor.update(renderer);
		break;

	case LEVEL_SELECT:
		break;
	}
	ShaderManager::update();
	renderer.gfx.drawDebug();
}
