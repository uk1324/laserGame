#pragma once

#include <game/Editor.hpp>
#include <game/Game.hpp>
#include <game/LevelSelect.hpp>
#include <game/MainMenu.hpp>
#include <game/SettingsManager.hpp>
#include <game/GameSave.hpp>
#include <game/Levels.hpp>
#include <game/GameAudio.hpp>

enum class TransitionEffectType {
	SLIDING,
	FADING
};

struct MainLoop {
	MainLoop();
	void update();

	GameRenderer renderer;
	GameAudio audio;
	Editor editor;
	Game game;
	LevelSelect levelSelect;
	MainMenu mainMenu;
	SettingsManager settingsManager;
	GameSave gameSave;
	Levels levels;

	void propagateSettings(const Settings& settings);

	enum class State {
		MAIN_MENU,
		SOUND_SETTINGS,
		GAME, 
		EDITOR, 
		LEVEL_SELECT, 
		TRANSITION_TO_LEVEL,
		STATELESS_TRANSITION,
		CONGRATULATIONS,
	} state = State::EDITOR;

	void switchToState(State currentState, State newState);
	void stateUpdate(State state);

	struct TransitionToLevelState {
		State startState;
		f32 t = 0.0f;
		LevelIndex levelIndex = 0;
		TransitionEffectType transitionEffect;
	} transitionToLevel;
	void doTransitionToLevel(i32 levelIndex, TransitionEffectType transitionEffect);

	struct StatelessTransitionState {
		f32 t = 0.0f;
		State startState;
		State endState;
	} statelessTransition;

	void doBasicTransition(State endState);
};