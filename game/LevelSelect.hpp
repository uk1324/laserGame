#pragma once 

#include <game/Ui.hpp>
#include <variant>

struct LevelSelect {
	struct ResultGoToLevel {
		//i32 index;
	};
	struct ResultNone {};
	using Result = std::variant<ResultNone, ResultGoToLevel>;

	Result update(GameRenderer& renderer);
};