#include "MainLoop.hpp"
#include <engine/Input/Input.hpp>
#include <engine/Window.hpp>
#include <engine/gfx/ShaderManager.hpp>
#include <glad/glad.h>
#include <Overloaded.hpp>
#include <engine/Math/Interpolation.hpp>

const auto mid = 0.5f;


bool updateTAndCheckIfAtMid(f32& t) {
	const auto oldT = t;
	t += Constants::dt;

	if (oldT < mid && t >= mid) {
		t = mid;
		return true;
	}
	return false;
}

void updateT(f32& t) {
	t += Constants::dt;
}

void renderFadingTransition(GameRenderer& r, f32 t) {
	Vec2 points[] = { Vec2(0.0f, 0.0f), Vec2(mid, 1.0f), Vec2(1.0f, 0.0f) };
	const auto a = piecewiseLinearSample(constView(points), t);

	Ui::rectPosSizeFilled(r, Vec2(0.0f), Vec2(1.0f), Vec4(Color3::BLACK, a));
	r.gfx.drawFilledTriangles();
}

MainLoop::MainLoop()
	: renderer(GameRenderer::make()) {

	settingsManager.tryLoadSettings();
	mainMenu.setSoundSettings(settingsManager.settings.audio);
	//settingsManager.
}
#include <game/GameSerialization.hpp>

void MainLoop::update() {
	if (Input::isKeyDown(KeyCode::TAB)) {
		switch (state) {
			using enum State;
		case GAME:
			state = State::EDITOR;
			break;

		case EDITOR: {
			state = State::GAME;
			const auto level = saveGameLevelToJson(editor.e);
			game.tryLoadLevelFromJson(level);
			break;
		}
			

		default:
			break;
		}
	}

	renderer.gfx.camera.aspectRatio = Window::aspectRatio();
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, GLsizei(Window::size().x), GLsizei(Window::size().y));

	switch (state) {
		using enum State;

	case MAIN_MENU: {
		const auto result = mainMenu.update(renderer);
		
		switch (result) {
			using enum MainMenu::Result;

		case GO_TO_SOUND_SETTINGS:
			doBasicTransition(State::SOUND_SETTINGS);
			break;

		case GO_TO_LEVEL_SELECT:
			doBasicTransition(State::LEVEL_SELECT);
			break;

		case NONE:
			break;
		}
		break;
	}

	case SOUND_SETTINGS: {
		const auto result = mainMenu.soundSettingsUpdate(renderer);
		switch (result) {
			using enum MainMenu::SoundSettingsResult;
		case GO_BACK:
			settingsManager.settings.audio = mainMenu.getSoundSettings();
			settingsManager.trySaveSettings();
			doBasicTransition(State::MAIN_MENU);
			break;

		case NONE:
			break;
		}
		break;
	}

	case GAME: {
		const auto result = game.update(renderer);
		std::visit(overloaded{
			[](const Game::ResultNone&) {},
			[&](const Game::ResultGoToLevel& r) {
				transitionFromLevelToLevel.t = 0;
				state = State::TRANSITION_FROM_LEVEL_TO_LEVEL;
			}
		}, result);
		break;
	}
		 

	case EDITOR:
		editor.update(renderer);
		break;

	case LEVEL_SELECT: {
		const auto result = levelSelect.update(renderer);
		std::visit(overloaded{
			[](const LevelSelect::ResultNone&) {},
			[&](const LevelSelect::ResultGoToLevel& r) {
				transitionFromLevelSelectToLevel.t = 0;
				state = State::TRANSITION_FROM_LEVEL_SELECT_TO_LEVEL;
			}
		}, result);
		break;
	}
		

	case TRANSITION_FROM_LEVEL_SELECT_TO_LEVEL: {
		auto& s = transitionFromLevelSelectToLevel;

		if (updateTAndCheckIfAtMid(s.t)) {
			// load level
		}

		if (s.t < mid) {
			levelSelect.update(renderer);
		} else {
			game.update(renderer);
		}

		if (s.t >= 1.0f) {
			state = State::GAME;
		}

		renderFadingTransition(renderer, s.t);
		break;
	}

	case TRANSITION_FROM_LEVEL_TO_LEVEL: {
		auto& s = transitionFromLevelToLevel;

		if (updateTAndCheckIfAtMid(s.t)) {
			// load level
		}

		game.update(renderer);

		if (s.t >= 1.0f) {
			state = State::GAME;
		}

		const auto offset = Vec2(lerp(-1.0f, 1.0f, s.t), 0.0f);
		const auto min = Vec2(-0.5f) + offset;
		const auto max = Vec2(0.5f) + offset;
		Ui::rectMinMaxFilled(renderer, min, max, Color3::BLACK);
		renderer.gfx.drawFilledTriangles();
		break;
	}

	case STATELESS_TRANSITION: {
		auto& s = statelessTransition;

		if (updateTAndCheckIfAtMid(s.t)) {
			renderer.changeTextColorRngSeed();
		}

		if (s.t < 0.5f) {
			stateUpdate(s.startState);
		} else {
			stateUpdate(s.endState);
		}

		if (s.t >= 1.0f) {
			state = s.endState;
		}
		renderFadingTransition(renderer, s.t);
		break;
	}

	}
	ShaderManager::update();
	renderer.gfx.drawDebug();
}

void MainLoop::stateUpdate(State state) {
	switch (state) {
		// These don't inspect the result on purpose. This function should be called from the transition functions and it doesn't make sense to begin a transition while one is already happening.
		using enum MainLoop::State;
	case MAIN_MENU: mainMenu.update(renderer); break;
	case SOUND_SETTINGS: mainMenu.soundSettingsUpdate(renderer); break;
	case EDITOR: editor.update(renderer); break;
	case LEVEL_SELECT: levelSelect.update(renderer); break;
		// There should be a level loaded before transitioning into game.
	case GAME: game.update(renderer); break;

		// It doesn't make sense to transition from or into these states.
	case TRANSITION_FROM_LEVEL_SELECT_TO_LEVEL:
	case TRANSITION_FROM_LEVEL_TO_LEVEL:
	case STATELESS_TRANSITION:
		CHECK_NOT_REACHED(); break;
	}
}

void MainLoop::doBasicTransition(State endState) {
	statelessTransition = StatelessTransitionState{
		.t = 0.0f,
		.startState = state,
		.endState = endState
	};
	state = State::STATELESS_TRANSITION;
}
