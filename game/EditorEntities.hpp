#pragma once

#include <engine/Math/Vec2.hpp>
#include <game/EntityArray.hpp>

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

enum class EditorEntityType {
	WALL,
	LASER,
	MIRROR,
};

struct EditorEntityId {
	EditorEntityType type;
	u32 index;
	u32 version;

	explicit EditorEntityId(const EditorWallId& id);
	explicit EditorEntityId(const EditorLaserId& id);
	explicit EditorEntityId(const EditorMirrorId& id);

	EditorWallId wall() const;
	EditorLaserId laser() const;
	EditorMirrorId mirror() const;

	bool operator==(const EditorEntityId&) const = default;
};