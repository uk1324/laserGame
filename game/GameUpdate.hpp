#pragma once

#include <game/EditorEntities.hpp>

struct GameState {
	void update(
		WallArray& walls,
		LaserArray& lasers,
		MirrorArray& mirrors,
		TargetArray& targets,
		PortalPairArray& portalPairs,
		TriggerArray& triggers,
		DoorArray& doors);

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
	std::optional<TriggerInfo> triggerInfo(TriggerArray& triggers, i32 triggerIndex);

	i32 maxReflections = 32;
};
