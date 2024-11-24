#include <game/Levels.hpp>
#include "Levels.hpp"

Levels::Levels() {
#define A(levelName) \
	levels.push_back(LevelInfo{ \
		.path = "assets/levels/" levelName, \
		.name = levelName, \
	});

	A("0")
	A("1")

#undef A
}

std::optional<const LevelInfo&> Levels::tryGetLevelInfo(LevelIndex index) const {
	if (index >= levels.size()) {
		return std::nullopt;
	}
	return levels[index];
}