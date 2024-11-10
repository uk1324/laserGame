#pragma once

#include <game/EditorEntities.hpp>
#include <game/EditorActions.hpp>

struct LaserGrabTool {
	enum class LaserPart {
		ORIGIN, DIRECTION,
	};

	struct Grabbed {
		EditorLaserId id;
		LaserPart part;
		EditorLaser grabStartState;
		Vec2 offset;
	};

	std::optional<Grabbed> grabbed;

	void update(LaserArray& lasers, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints);
};