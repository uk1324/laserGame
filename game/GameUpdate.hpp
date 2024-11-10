#pragma once

#include <game/EditorEntities.hpp>

struct GameState {
	void update(GameEntities& e);

	struct Segment {
		Vec2 endpoints[2];
		Vec3 color; // space inefficient
		bool ignore = false;
	};
	std::vector<Segment> laserSegmentsToDraw;

	struct TriggerInfo {
		Vec3 color;
		bool active;
	};
	static std::optional<TriggerInfo> triggerInfo(TriggerArray& triggers, i32 triggerIndex);

	i32 maxReflections = 32;

	Vec2 focus[2]{ Vec2(0.2f, 0.0f), Vec2(-0.2f, 0.0f) };
	f32 eccentricity = 0.1f;
	struct Seg {
		Vec2 a;
		Vec2 b;
	};
	std::vector<Seg> segments;
};
