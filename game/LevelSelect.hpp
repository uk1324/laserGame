#pragma once 

#include <game/GameUi.hpp>
#include <game/GameSave.hpp>
#include <variant>

struct LevelSelect {
	LevelSelect();

	struct ResultGoToLevel {
		i32 index;
	};
	struct ResultGoBack {};
	struct ResultNone {};
	using Result = std::variant<ResultNone, ResultGoToLevel, ResultGoBack>;


	std::vector<GameUiButton> buttons;

	GameUiButton backButton;

	static constexpr auto squareCountY = 4;
	static constexpr auto squareCountX = 6;

	Result update(GameRenderer& renderer, const Levels& levels, const GameSave& save);
};