#include <game/Levels.hpp>
#include "Levels.hpp"

Levels::Levels() {
#define A(levelName) \
	levels.push_back(LevelInfo{ \
		.path = "assets/levels/" levelName, \
		.name = levelName, \
	});

	A("1")
	A("2")
	A("3")
	A("4")
	A("5")
	A("6")
	A("7")
	A("8")
	A("9")
	A("10")
	A("11")
	A("12")

#undef A
}

std::optional<const LevelInfo&> Levels::tryGetLevelInfo(LevelIndex index) const {
	if (index >= levels.size()) {
		return std::nullopt;
	}
	return levels[index];
}