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

void renderSlidingTransition(GameRenderer& r, f32 t) {
	const auto offset = Vec2(lerp(-1.0f, 1.0f, t), 0.0f);
	const auto min = Vec2(-0.5f) + offset;
	const auto max = Vec2(0.5f) + offset;
	Ui::rectMinMaxFilled(r, min, max, Color3::BLACK);
	r.gfx.drawFilledTriangles();
}

void renderTransition(GameRenderer& r, f32 t, TransitionEffectType type) {
	switch (type) {
		using enum TransitionEffectType;

	case SLIDING:
		renderSlidingTransition(r, t);
		break;

	case FADING:
		renderFadingTransition(r, t);
		break;
	}
}

void updateTransition(GameRenderer& r, GameAudio& a, f32 previousT, f32 t, TransitionEffectType type) {
	switch (type) {
		using enum TransitionEffectType;
	case SLIDING: {
		renderSlidingTransition(r, t);
		const auto soundEffectT = 0.05f;
		if (previousT <= soundEffectT && t >= soundEffectT) {
			a.playSoundEffect(a.transitionSwooshSound);
		}
		break;
	}
		
	case FADING:
		break;
	}
}

MainLoop::MainLoop()
	: renderer(GameRenderer::make()) {

	settingsManager.tryLoadSettings();
	mainMenu.settings = settingsManager.settings;

	propagateSettings(settingsManager.settings);

	gameSave.tryLoad();
}
#include <game/GameSerialization.hpp>

void MainLoop::update() {
	auto previewEditorLevelInGame = [this]() {
		switchToState(state, State::GAME);
		const auto level = saveGameLevelToJson(editor.e);
		game.tryLoadEditorPreviewLevel(level);
	};

	if (Input::isKeyDown(KeyCode::TAB)) {
		switch (state) {
			using enum State;
		case GAME:
			if (game.isEditorPreviewLevelLoaded()) {
				switchToState(state, State::EDITOR);
			}
			break;

		case EDITOR: {
			previewEditorLevelInGame();
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

		case PLAY: {
			const auto firstUncompletedLevel = gameSave.firstUncompletedLevel(levels);
			if (firstUncompletedLevel.has_value()) {
				doTransitionToLevel(*firstUncompletedLevel, TransitionEffectType::FADING);
			} else {
				doBasicTransition(State::CONGRATULATIONS);
			}
			break;
		}

		case GO_TO_SOUND_SETTINGS:
			doBasicTransition(State::SOUND_SETTINGS);
			break;

		case GO_TO_LEVEL_SELECT:
			doBasicTransition(State::LEVEL_SELECT);
			break;

		case GO_TO_EDITOR:
			state = State::EDITOR;
			break;

		case NONE:
			break;
		}
		break;
	}

	case SOUND_SETTINGS: {
		const auto result = mainMenu.settingsUpdate(renderer);
		switch (result) {
			using enum MainMenu::SettingsResult;
		case GO_BACK:
			settingsManager.settings = mainMenu.settings;
			settingsManager.trySaveSettings();
			propagateSettings(settingsManager.settings);
			doBasicTransition(State::MAIN_MENU);
			break;

		case PROPAGATE_SETTINGS:
			propagateSettings(mainMenu.settings);
			break;

		case NONE:
			break;
		}
		break;
	}

	case GAME: {
		const auto result = game.update(renderer, audio);
		if (Input::isKeyDown(KeyCode::H)) {
			doTransitionToLevel(0, TransitionEffectType::SLIDING);
		}

		std::visit(overloaded{
			[](const Game::ResultNone&) {},
			[&](const Game::ResultGoToNextLevel&) {
				if (!game.currentLevel.has_value()) {
					CHECK_NOT_REACHED();
					return;
					/*const auto firstUncompletedLevel = gameSave.firstUncompletedLevel(levels);
					if (firstUncompletedLevel.has_value()) {
						doTransitionToLevel(*firstUncompletedLevel, TransitionEffectType::SLIDING);
					}*/
				} 
				//gameSave.completedLevels.insert(levels.levels[*game.currentLevel])
				std::optional<i32> firstUncompletedLevelAfterCurrent = gameSave.firstUncompletedLevelAfter(*game.currentLevel, levels);

				gameSave.markAsCompleted(*game.currentLevel, levels);
				gameSave.trySave();
				if (firstUncompletedLevelAfterCurrent.has_value()) {
					doTransitionToLevel(*firstUncompletedLevelAfterCurrent, TransitionEffectType::SLIDING);
					return;
				}

				if (const auto firstUncompletedLevel = gameSave.firstUncompletedLevel(levels); firstUncompletedLevel.has_value()) {
					doTransitionToLevel(*firstUncompletedLevel, TransitionEffectType::SLIDING);
					return;
				} 

				const auto nextLevel = *game.currentLevel + 1;
				if (nextLevel >= levels.levels.size()) {
					doBasicTransition(State::CONGRATULATIONS);
					return;
				}

				doTransitionToLevel(nextLevel, TransitionEffectType::SLIDING);
			},
			[&](const Game::ResultSkipLevel&) {
				if (!game.currentLevel.has_value()) {
					CHECK_NOT_REACHED();
					return;
				}
				std::optional<i32> firstUncompletedLevelAfterCurrent = gameSave.firstUncompletedLevelAfter(*game.currentLevel, levels);

				if (firstUncompletedLevelAfterCurrent.has_value()) {
					doTransitionToLevel(*firstUncompletedLevelAfterCurrent, TransitionEffectType::SLIDING);
					return;
				}

				const auto nextLevel = *game.currentLevel + 1;
				if (nextLevel >= levels.levels.size()) {
					doBasicTransition(State::CONGRATULATIONS);
					return;
				}

				doTransitionToLevel(nextLevel, TransitionEffectType::SLIDING);
			},
			[&](const Game::ResultGoToMainMenu) {
				doBasicTransition(State::MAIN_MENU);
			}
		}, result);
		break;
	}
		 

	case EDITOR: {
		const auto result = editor.update(renderer);
		switch (result) {
			using enum Editor::Result;

		case GO_TO_MAIN_MENU:
			state = State::MAIN_MENU;
			break;

		case PREVIEW_LEVEL:
			previewEditorLevelInGame();
			break;

		case NONE:
			break;
		}
		break;
	}
		

	case LEVEL_SELECT: {
		const auto result = levelSelect.update(renderer, levels, gameSave);
		std::visit(overloaded{
			[](const LevelSelect::ResultNone&) {},
			[&](const LevelSelect::ResultGoToLevel& r) {
				doTransitionToLevel(r.index, TransitionEffectType::FADING);
			},
			[&](const LevelSelect::ResultGoBack&) {
				doBasicTransition(State::MAIN_MENU);
			}
		}, result);
		break;
	}
		
	case STATELESS_TRANSITION: {
		auto& s = statelessTransition;

		if (updateTAndCheckIfAtMid(s.t)) {
			renderer.randomize();
		}

		if (s.t < 0.5f) {
			stateUpdate(s.startState);
		} else {
			stateUpdate(s.endState);
		}

		if (s.t >= 1.0f) {
			switchToState(s.startState, s.endState);
		}
		renderFadingTransition(renderer, s.t);
		break;
	}

	case TRANSITION_TO_LEVEL: {
		auto& s = transitionToLevel;

		const auto previousT = s.t;
		if (updateTAndCheckIfAtMid(s.t)) {
			renderer.randomize();
			game.tryLoadGameLevel(levels, s.levelIndex);
		}

		if (s.t < mid) {
			stateUpdate(s.startState);
		} else {
			game.update(renderer, audio);
		}

		if (s.t >= 1.0f) {
			switchToState(s.startState, State::GAME);
		}
		updateTransition(renderer, audio, previousT, s.t, s.transitionEffect);
		renderTransition(renderer, s.t, s.transitionEffect);
		break;
	}

	case CONGRATULATIONS: {
		const auto result = mainMenu.congratulationsScreenUpdate(renderer);
		switch (result) {
			using enum MainMenu::CongratulationsScreenResult;

		case NONE:
			break;

		case GO_TO_LEVEL_SELECT:
			doBasicTransition(State::LEVEL_SELECT);
			break;

		case GO_TO_MAIN_MENU:
			doBasicTransition(State::MAIN_MENU);
			break;
		}
		break;
	}

	}
	ShaderManager::update();
	renderer.gfx.drawDebug();
}

void MainLoop::propagateSettings(const Settings& settings) {
	audio.setSoundEffectVolume(settings.audio.soundEffectVolume);
	renderer.settings = settings.graphics;
	Window::setFullscreen(settings.graphics.fullscreen);
}

void MainLoop::switchToState(State currentState, State newState) {
	switch (currentState) {
		using enum State;
	case GAME:
		game.onSwitchFromGame(audio);
		break;

	default:
		break;
	}

	state = newState;

	switch (state) {
		using enum State;
	case GAME:
		game.onSwitchToGame(audio);
		break;

	default:
		break;
	}
}

void MainLoop::stateUpdate(State state) {
	switch (state) {
		// These don't inspect the result on purpose. This function should be called from the transition functions and it doesn't make sense to begin a transition while one is already happening.
		using enum MainLoop::State;
	case MAIN_MENU: mainMenu.update(renderer); break;
	case SOUND_SETTINGS: mainMenu.settingsUpdate(renderer); break;
	case EDITOR: editor.update(renderer); break;
	case LEVEL_SELECT: levelSelect.update(renderer, levels, gameSave); break;
		// There should be a level loaded before transitioning into game.
	case GAME: game.update(renderer, audio); break;
	case CONGRATULATIONS: mainMenu.congratulationsScreenUpdate(renderer); break;

		// It doesn't make sense to transition from or into these states.
	case TRANSITION_TO_LEVEL:
	case STATELESS_TRANSITION:
		CHECK_NOT_REACHED(); break;
	}
}

void MainLoop::doTransitionToLevel(i32 levelIndex, TransitionEffectType transitionEffect) {
	transitionToLevel = TransitionToLevelState{
		.startState = state,
		.t = 0.0f,
		.levelIndex = levelIndex,
		.transitionEffect = transitionEffect,
	};
	state = State::TRANSITION_TO_LEVEL;
}

void MainLoop::doBasicTransition(State endState) {
	statelessTransition = StatelessTransitionState{
		.t = 0.0f,
		.startState = state,
		.endState = endState
	};
	state = State::STATELESS_TRANSITION;
}
