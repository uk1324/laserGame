#pragma once

#include <engine/Math/Vec2.hpp>
#include <game/EntityArray.hpp>
#include <game/Stereographic.hpp>
#include <engine/Math/Color.hpp>

enum class EditorWallType {
	REFLECTING,
	ABSORBING
};

void wallTypeCombo(const char* label, EditorWallType& type);

struct EditorWall {
	struct DefaultInitialize {
		EditorWall operator()();
	};

	Vec2 endpoints[2];
	EditorWallType type;
};
using EditorWallId = EntityArrayId<EditorWall>;

struct EditorLaser {
	struct DefaultInitialize {
		EditorLaser operator()();
	};

	Vec2 position;
	f32 angle;
	Vec3 color;
	bool positionLocked;

	static constexpr Vec3 defaultColor = Color3::CYAN;
};

void editorLaserColorCombo(const char* label, Vec3& color);

using EditorLaserId = EntityArrayId<EditorLaser>;

std::array<Vec2, 2> rotatableSegmentEndpoints(Vec2 center, f32 normalAngle, f32 length);

struct EditorMirror {
	struct DefaultInitialize {
		EditorMirror operator()();
	};

	EditorMirror(Vec2 center, f32 normalAngle, f32 length, bool positionLocked);

	Vec2 center;
	f32 normalAngle;
	f32 length;
	bool positionLocked;

	std::array<Vec2, 2> calculateEndpoints() const;
};

void editorMirrorLengthInput(f32& length);

using EditorMirrorId = EntityArrayId<EditorMirror>;

struct EditorTarget {
	struct DefaultInitialize {
		EditorTarget operator()();
	};

	Circle calculateCircle() const;

	static constexpr auto defaultRadius = 0.1f;
	Vec2 position;
	f32 radius;
};

void editorTargetRadiusInput(f32& radius);

using EditorTargetId = EntityArrayId<EditorTarget>;

enum class EditorPortalWallType {
	PORTAL, REFLECTING, ABSORBING,
};

struct EditorPortal {
	Vec2 center;
	f32 normalAngle;
	EditorPortalWallType wallType;

	static constexpr auto defaultLength = 0.6f;

	std::array<Vec2, 2> endpoints() const;
};

struct EditorPortalPair {
	struct DefaultInitialize {
		EditorPortalPair operator()();
	};
	EditorPortal portals[2];
};

using EditorPortalPairId = EntityArrayId<EditorPortalPair>;

//struct EditorPortal {
//	Vec2 position;
//	f32 angle;
//	i32 index
//};
// The above version could allow cloning lasers with portals, by making 3 portals with the same index for example. One big issue with this is that doors work based on order and there would need to be an order found for which portal the laser goes through first. Maybe based on distance. It could be cool, but it seems unnescecarily complicated.

//struct EditorDoor {
//	i32 triggerIndex;
//	Vec2 endpoints[2];
//};
//
//struct EditorTriggerOrb {
//	i32 index;
//	Vec2 position;
//};

enum class EditorEntityType {
	WALL,
	LASER,
	MIRROR,
	TARGET,
	PORTAL_PAIR
};

struct EditorEntityId {
	EditorEntityType type;
	u32 index;
	u32 version;

	explicit EditorEntityId(const EditorWallId& id);
	explicit EditorEntityId(const EditorLaserId& id);
	explicit EditorEntityId(const EditorMirrorId& id);
	explicit EditorEntityId(const EditorTargetId& id);
	explicit EditorEntityId(const EditorPortalPairId& id);

	EditorWallId wall() const;
	EditorLaserId laser() const;
	EditorMirrorId mirror() const;
	EditorTargetId target() const;
	EditorPortalPairId portalPair() const;

	bool operator==(const EditorEntityId&) const = default;
};