#pragma once

#include <game/EditorEntities.hpp>

struct GameState {
	void snapObjectPositionsInsideBoundary(GameEntities& e);
	void update(GameEntities& e);
	void laserUpdate(EditorLaser& laser, GameEntities& e, const std::vector<Vec2>& corners);

	struct Segment {
		Vec2 endpoints[2];
		Vec3 color; // space inefficient
		bool ignore = false;
	};
	std::vector<Segment> laserSegmentsToDraw;
	bool anyTargetsTurnedOn = false;

	struct TriggerInfo {
		Vec3 color;
		Vec3 currentColor;
		bool active;
	};
	static std::optional<TriggerInfo> triggerInfo(TriggerArray& triggers, i32 triggerIndex);

	//i32 maxReflections = 32;
	i32 maxReflections = 100;

	Vec2 focus[2]{ Vec2(0.2f, 0.0f), Vec2(-0.2f, 0.0f) };
	f32 eccentricity = 0.1f;
	struct Seg {
		Vec2 a;
		Vec2 b;
	};
	std::vector<Seg> segments;
};
