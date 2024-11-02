#pragma once

#include <engine/Math/Vec2.hpp>
#include <game/EntityArray.hpp>
#include <game/Stereographic.hpp>
#include <engine/Math/Color.hpp>
#include <View.hpp>

struct ColorEntry {
	Vec3 color;
	const char* name;
};
void colorCombo(const char* label, View<const ColorEntry> colors, ColorEntry defaultColor, Vec3& selectedColor);

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

	//static constexpr ColorEntry defaultColor{ Color3::BLUE, "blue" };
	static constexpr ColorEntry defaultColor{ Color3::CYAN, "cyan" };
};

void editorLaserColorCombo(Vec3& color);

using EditorLaserId = EntityArrayId<EditorLaser>;

std::array<Vec2, 2> rotatableSegmentEndpoints(Vec2 center, f32 normalAngle, f32 length);

enum class EditorMirrorWallType {
	REFLECTING,
	ABSORBING
};

struct EditorMirror {
	struct DefaultInitialize {
		EditorMirror operator()();
	};

	EditorMirror(Vec2 center, f32 normalAngle, f32 length, bool positionLocked, EditorMirrorWallType wallType);

	Vec2 center;
	f32 normalAngle;
	f32 length;
	bool positionLocked;
	EditorMirrorWallType wallType;

	std::array<Vec2, 2> calculateEndpoints() const;
};

void editorMirrorLengthInput(f32& length);
void editorMirrorWallTypeInput(EditorMirrorWallType& type);

using EditorMirrorId = EntityArrayId<EditorMirror>;

struct EditorTarget {
	struct DefaultInitialize {
		EditorTarget operator()();
	};

	Circle calculateCircle() const;

	static constexpr auto defaultRadius = 0.1f;
	Vec2 position;
	f32 radius;

	bool activated = false;
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
	bool positionLocked;
	bool rotationLocked;

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
struct EditorTrigger {
	struct DefaultInitialize {
		EditorTrigger operator()();
	};

	Vec2 position;
	Vec3 color;
	i32 index;

	bool activated = false;

	Circle circle() const;

	static constexpr auto defaultRadius = 0.1f;
	static constexpr ColorEntry defaultColor{ Vec3(0.0f, 1.0f, 0.5f), "spring green"};
};

void editorTriggerColorCombo(Vec3& color);

using EditorTriggerId = EntityArrayId<EditorTrigger>;

enum class EditorEntityType {
	WALL,
	LASER,
	MIRROR,
	TARGET,
	PORTAL_PAIR,
	TRIGGER,
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
	explicit EditorEntityId(const EditorTriggerId& id);

	EditorWallId wall() const;
	EditorLaserId laser() const;
	EditorMirrorId mirror() const;
	EditorTargetId target() const;
	EditorPortalPairId portalPair() const;
	EditorTriggerId trigger() const;

	bool operator==(const EditorEntityId&) const = default;
};