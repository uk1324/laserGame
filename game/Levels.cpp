#include <game/Levels.hpp>
#include "Levels.hpp"
#include  <game/WorkingDirectory.hpp>

Levels::Levels() {
#define A(levelName) \
	levels.push_back(LevelInfo{ \
		.workingDirectoryPath = "assets/levels/" levelName, \
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
	A("13")
	A("14")
	A("15")
	A("16")
	A("17")
	A("18")
	A("19")
	A("20")
	A("21")
	A("22")
	A("23")
	A("24")

#undef A
}

std::optional<const LevelInfo&> Levels::tryGetLevelInfo(LevelIndex index) const {
	if (index >= levels.size()) {
		return std::nullopt;
	}
	return levels[index];
}

std::string LevelInfo::path() const {
	return (executableWorkingDirectory / workingDirectoryPath).string();
}
