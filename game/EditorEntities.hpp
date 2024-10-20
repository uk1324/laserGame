#pragma once

#include <engine/Math/Vec2.hpp>
#include <game/EntityArray.hpp>
#include <game/Stereographic.hpp>

struct EditorWall {
	struct DefaultInitialize {
		EditorWall operator()();
	};

	Vec2 endpoints[2];
};
using EditorWallId = EntityArrayId<EditorWall>;

struct EditorLaser {
	struct DefaultInitialize {
		EditorLaser operator()();
	};

	Vec2 position;
	f32 angle;
};

using EditorLaserId = EntityArrayId<EditorLaser>;

struct EditorMirror {
	struct DefaultInitialize {
		EditorMirror operator()();
	};

	Vec2 center;
	f32 normalAngle;

	std::array<Vec2, 2> calculateEndpoints() const;
};

using EditorMirrorId = EntityArrayId<EditorMirror>;

struct EditorTarget {
	struct DefaultInitialize {
		EditorTarget operator()();
	};

	Circle calculateCircle() const;

	Vec2 position;
};

using EditorTargetId = EntityArrayId<EditorTarget>;

enum class EditorEntityType {
	WALL,
	LASER,
	MIRROR,
	TARGET,
};

struct EditorEntityId {
	EditorEntityType type;
	u32 index;
	u32 version;

	explicit EditorEntityId(const EditorWallId& id);
	explicit EditorEntityId(const EditorLaserId& id);
	explicit EditorEntityId(const EditorMirrorId& id);
	explicit EditorEntityId(const EditorTargetId& id);

	EditorWallId wall() const;
	EditorLaserId laser() const;
	EditorMirrorId mirror() const;
	EditorTargetId target() const;

	bool operator==(const EditorEntityId&) const = default;
};