#pragma once

#include <game/EditorEntities.hpp>
#include <game/EditorActions.hpp>

bool isGrabReleased();

enum class RotatableSegmentGizmoType {
	TRANSLATION, ROTATION_0, ROTATION_1
};
struct GrabbedRotatableSegment {
	Vec2 grabOffset;
	RotatableSegmentGizmoType grabbedGizmo;
};
std::optional<GrabbedRotatableSegment> rotatableSegmentCheckGrab(Vec2 center, f32 normalAngle, f32 length,
	Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);
void grabbedRotatableSegmentUpdate(RotatableSegmentGizmoType type, Vec2 grabOffset,
	Vec2& center, f32& normalAngle,
	Vec2 cursorPos, bool cursorExact);

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

struct MirrorGrabTool {
	struct Grabbed {
		EditorMirrorId id;
		EditorMirror grabStartState;
		GrabbedRotatableSegment grabbed;
	};

	std::optional<Grabbed> grabbed;

	void update(MirrorArray& mirrors, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints);
};

struct PortalGrabTool {
	struct Grabbed {
		EditorPortalPairId id;
		i32 portalIndex;
		EditorPortalPair grabStartState;
		GrabbedRotatableSegment grabbed;
	};
	std::optional<Grabbed> grabbed;

	void update(PortalPairArray& portalPairs, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints);
};