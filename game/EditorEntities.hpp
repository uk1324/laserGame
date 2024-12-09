#pragma once

#include <engine/Math/Vec2.hpp>
#include <game/EntityArray.hpp>
#include <game/Stereographic.hpp>
#include <engine/Math/Color.hpp>
#include <View.hpp>

// Storing intial positions so the errors in the static objects don't accumulate from movement. If I store the intial positions then I can apply the transformation to the initial position. One objects that can be modified like mirrors the error can accumulate, but they won't break the level, because the user can move them anyway.

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
Vec3 currentOrbColor(Vec3 color, f32 activationAnimationT);

struct EditorWall {
	struct DefaultInitialize {
		EditorWall operator()();
	};

	bool operator==(const EditorWall&) const = default;

	Vec2 endpoints[2];
	Vec2 initialEndpoints[2];
	EditorWallType type;
};
using EditorWallId = EntityArrayId<EditorWall>;
using WallArray = EntityArray<EditorWall, EditorWall::DefaultInitialize>;

struct EditorLaser {
	struct DefaultInitialize {
		EditorLaser operator()();
	};

	bool operator==(const EditorLaser&) const = default;

	Vec2 position;
	f32 angle;
	Vec3 color;
	bool positionLocked;
	bool rotationLocked;

	static constexpr ColorEntry defaultColor{ Color3::BLUE, "blue" };
	//static constexpr ColorEntry defaultColor{ Color3::CYAN, "cyan" };
};
using LaserArray = EntityArray<EditorLaser, EditorLaser::DefaultInitialize>;

void editorLaserColorCombo(Vec3& color);
struct LaserGrabPoint {
	// The rendering code uses that visual point and the grabbing code uses the actual point. The difference isn't too big so it isn't very noticible that a different point is grabbed. Might be possible to it so that the actual is grabbed like in mirrors for example. Not sure if there will be any issues, because lasers will use eucledian distance instead of stereographic distance.
	//Vec2 point;
	Vec2 visualPoint;
	Vec2 visualTangent;
};

LaserGrabPoint laserDirectionGrabPoint(const EditorLaser& laser);

struct LaserArrowhead {
	//Vec2 actualTip;
	Vec2 tip;
	Vec2 ears[2];

	f32 distanceTo(Vec2 v) const;
};
LaserArrowhead laserArrowhead(const EditorLaser& laser);

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

	bool operator==(const EditorMirror&) const = default;

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
using MirrorArray = EntityArray<EditorMirror, EditorMirror::DefaultInitialize>;

struct EditorTarget {
	struct DefaultInitialize {
		EditorTarget operator()();
	};

	bool operator==(const EditorTarget&) const = default;

	Circle calculateCircle() const;

	static constexpr auto defaultRadius = 0.1f;
	Vec2 position;
	Vec2 initialPosition;
	f32 radius;

	bool activated = false;
	bool activatedLastFrame = false;
	f32 activationAnimationT = 0.0f;
};

void editorTargetRadiusInput(f32& radius);

using EditorTargetId = EntityArrayId<EditorTarget>;
using TargetArray = EntityArray<EditorTarget, EditorTarget::DefaultInitialize>;

enum class EditorPortalWallType {
	PORTAL, REFLECTING, ABSORBING,
};

struct EditorPortal {
	Vec2 center;
	f32 normalAngle;
	EditorPortalWallType wallType;
	bool positionLocked;
	bool rotationLocked;

	bool operator==(const EditorPortal&) const = default;

	static constexpr auto defaultLength = 0.6f;

	std::array<Vec2, 2> endpoints() const;
};

struct EditorPortalPair {
	struct DefaultInitialize {
		EditorPortalPair operator()();
	};
	bool operator==(const EditorPortalPair&) const = default;

	EditorPortal portals[2];
};

using EditorPortalPairId = EntityArrayId<EditorPortalPair>;
using PortalPairArray = EntityArray<EditorPortalPair, EditorPortalPair::DefaultInitialize>;

//struct EditorPortal {
//	Vec2 position;
//	f32 angle;
//	i32 index
//};
// The above version could allow cloning lasers with portals, by making 3 portals with the same index for example. One big issue with this is that doors work based on order and there would need to be an order found for which portal the laser goes through first. Maybe based on distance. It could be cool, but it seems unnescecarily complicated.

struct EditorTrigger {
	struct DefaultInitialize {
		EditorTrigger operator()();
	};

	bool operator==(const EditorTrigger&) const = default;

	Vec2 position;
	Vec2 initialPosition;
	Vec3 color;
	i32 index;

	bool activated = false;
	f32 activationAnimationT = 0.0f;

	Circle circle() const;

	static constexpr auto defaultRadius = 0.1f;
	static constexpr ColorEntry defaultColor{ Vec3(0.0f, 1.0f, 0.5f), "spring green"};
};
using EditorTriggerId = EntityArrayId<EditorTrigger>;
using TriggerArray = EntityArray<EditorTrigger, EditorTrigger::DefaultInitialize>;

void editorTriggerColorCombo(Vec3& color);
void editorTriggerIndexInput(const char* label, i32& index);

struct EditorDoorSegment {
	Vec2 endpoints[2];
};

struct EditorDoor {
	struct DefaultInitialize {
		EditorDoor operator()();
	};

	bool operator==(const EditorDoor&) const = default;

	Vec2 endpoints[2];
	Vec2 initialEndpoints[2];
	i32 triggerIndex;
	bool openByDefault;

	f32 openingT = 0.0f;

	StaticList<EditorDoorSegment, 2> segments() const;
};
using EditorDoorId = EntityArrayId<EditorDoor>;
using DoorArray = EntityArray<EditorDoor, EditorDoor::DefaultInitialize>;

enum class EditorEntityType {
	WALL,
	LASER,
	MIRROR,
	TARGET,
	PORTAL_PAIR,
	TRIGGER,
	DOOR,
};

struct EditorEntity {
	explicit EditorEntity(EditorWall wall);
	explicit EditorEntity(EditorLaser laser);
	explicit EditorEntity(EditorMirror mirror);
	explicit EditorEntity(EditorTarget target);
	explicit EditorEntity(EditorPortalPair portalPair);
	explicit EditorEntity(EditorTrigger trigger);
	explicit EditorEntity(EditorDoor door);

	bool operator==(const EditorEntity& other);

	union {
		EditorWall wall;
		EditorLaser laser;
		EditorMirror mirror;
		EditorTarget target;
		EditorPortalPair portalPair;
		EditorTrigger trigger;
		EditorDoor door;
	};
	EditorEntityType type;
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
	explicit EditorEntityId(const EditorDoorId& id);

	EditorWallId wall() const;
	EditorLaserId laser() const;
	EditorMirrorId mirror() const;
	EditorTargetId target() const;
	EditorPortalPairId portalPair() const;
	EditorTriggerId trigger() const;
	EditorDoorId door() const;

	bool operator==(const EditorEntityId&) const = default;
};

struct LockedCells {
	std::vector<i32> cells;
	i32 segmentCount = 5;
	i32 ringCount = 5;
	
	struct CellBounds {
		f32 minA;
		f32 maxA;
		f32 minR;
		f32 maxR;

		bool containsPoint(Vec2 v) const;
	};

	std::optional<i32> getIndex(Vec2 pos) const;
	CellBounds cellBounds(i32 index) const;

	void reset();
};

// I decided put here all the things that relate to the level and put things that change every frame or are related to the update in game state.
struct GameEntities {
	WallArray walls;
	LaserArray lasers;
	MirrorArray mirrors;
	TargetArray targets;
	PortalPairArray portalPairs;
	TriggerArray triggers;
	DoorArray doors;
	LockedCells lockedCells;

	void reset();
};