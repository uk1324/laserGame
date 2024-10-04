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

enum class EditorEntityType {
	WALL,
};

struct EditorEntityId {
	EditorEntityType type;
	u32 index;
	u32 version;

	explicit EditorEntityId(const EditorWallId& id);

	EditorWallId wall() const;

	bool operator==(const EditorEntityId&) const = default;
};