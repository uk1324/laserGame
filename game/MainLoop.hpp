#pragma once

#include <game/Editor.hpp>
#include <game/Game.hpp>
#include <game/LevelSelect.hpp>
#include <game/MainMenu.hpp>
#include <game/SettingsManager.hpp>

struct MainLoop {
	MainLoop();
	void update();

	GameRenderer renderer;
	Editor editor;
	Game game;
	LevelSelect levelSelect;
	MainMenu mainMenu;
	SettingsManager settingsManager;

	enum class State {
		MAIN_MENU,
		SOUND_SETTINGS,
		GAME, 
		EDITOR, 
		LEVEL_SELECT, 
		TRANSITION_FROM_LEVEL_SELECT_TO_LEVEL,
		TRANSITION_FROM_LEVEL_TO_LEVEL,
		STATELESS_TRANSITION,
	} state = State::GAME;

	void stateUpdate(State state);

	struct TransitionFromLevelSelectToLevelState {
		f32 t = 0.0f;
		i32 levelIndex = 0;
	} transitionFromLevelSelectToLevel;

	struct TransitionFromLevelToLevelState {
		f32 t = 0.0f;
		i32 levelIndex = 0;
	} transitionFromLevelToLevel;

	struct StatelessTransitionState {
		f32 t = 0.0f;
		State startState;
		State endState;
	} statelessTransition;

	void doBasicTransition(State endState);
};