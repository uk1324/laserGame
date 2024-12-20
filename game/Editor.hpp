#pragma once

#include <game/GameRenderer.hpp>
#include <game/GrabTools.hpp>

struct EditorGridTool {
	EditorGridTool();

	enum class GridType {
		POLAR,
		HEMI_ICOSAHEDRAL,
		HEMI_DODECAHEDRAL,
	};

	struct EllipticIsometry {
		EllipticIsometry(f32 a0, f32 a1, f32 a2);
		f32 a[3];
		 Quat toQuaternion() const;
	};

	struct Ratio {
		Ratio(i32 numerator, i32 denominator);

		i32 numerator;
		i32 denominator;

		f32 toF32() const;
	};

	struct EllipticIsometryInput {
		static EllipticIsometryInput defaultInit();

		Ratio a[3];

		EllipticIsometry toEllipticIsometry() const;
	};

	bool showGrid = true;

	GridType gridType = GridType::POLAR;
	//GridType gridType = GridType::HEMI_DODECAHEDRAL;

	i32 polarGridLineCount = 16;
	i32 polarGridCircleCount = 10;

	struct SnapCursorResult {
		Vec2 cursorPos;
		bool snapped;
	};

	// First check if there is a vertex to snap to. If yes then snap, else try to snap to curve.
	SnapCursorResult snapCursor(Vec2 cursorPos);
	SnapCursorResult snapCursorToPolarGrid(Vec2 cursorPos);
	static SnapCursorResult snapCursorToShapeGrid(Vec2 cursorPos, const std::vector<Vec3>& vertices, const std::vector<ProjectivePolyhedron::Segment>& segments);

	void render(GameRenderer& renderer);
	void gui();

	static void renderEllipticSegment(GameRenderer& renderer, const ProjectivePolyhedron::Segment& segment, const std::vector<Vec3>& vertices, Vec3 color);

	EllipticIsometryInput hemiIcosahedronIsometry;
	enum class IcosahedronCoordinateSystem {
		VERTEX_CENTER,
		TRIANGLE_CENTER,
		EDGE_CENTER
	} icosahedronCoordinateSystem = IcosahedronCoordinateSystem::VERTEX_CENTER;
	Quat currentIcosahedronIsometry() const;

	EllipticIsometryInput hemiDodecahedronIsometry;
	enum class DodecahedronCoordinateSystem {
		VERTEX_CENTER,
		PENTAGON_CENTER,
		EDGE_CENTER
	} dodecahedronCoordinateSystem = DodecahedronCoordinateSystem::VERTEX_CENTER;
	Quat currentDodecahedronIsometry() const;
};

struct Editor {
	Editor();

	enum class Result {
		GO_TO_MAIN_MENU,
		PREVIEW_LEVEL,
		NONE,
	};

	Result update(GameRenderer& renderer);
	void undoRedoUpdate();

	enum class Tool {
		NONE,
		SELECT,
		WALL,
		LASER,
		MIRROR,
		TARGET,
		PORTAL_PAIR,
		TRIGGER,
		DOOR,
		//MODIFY_LOCKED_CELLS
	};

	struct WallLikeEntity {
		Vec2 endpoints[2];
	};

	struct WallLikeEntityCreateTool {
		std::optional<WallLikeEntity> update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured);
		std::optional<WallLikeEntity> preview(Vec2 cursorPos) const;
		void reset();

		std::optional<Vec2> endpoint;
	};

	struct GrabbedWallLikeEntity {
		i32 grabbedEndpointIndex;
		Vec2 grabOffset;
	};

	static std::optional<GrabbedWallLikeEntity> wallLikeEntityCheckGrab(View<const Vec2> endpoints,
		Vec2 cursorPos, bool& cursorCaptured);
	static void grabbedWallLikeEntityUpdate(const GrabbedWallLikeEntity& grabbed,
		View<Vec2> endpoints,
		Vec2 cursorPos, bool cursorExact);

	Tool selectedTool = Tool::WALL;

	struct SelectTool {
		std::optional<EditorSelected> selected;
		bool modifiedUsingGui = false;
	} selectTool;
	void selectToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	void saveSelectedEntityIfModified();

	struct WallCreateTool {
		WallLikeEntityCreateTool tool;

		std::optional<EditorWall> update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured);
		void render(GameRenderer& renderer, Vec2 cursorPos);

		EditorWallType wallType = EditorWallType::ABSORBING;
	} wallCreateTool;
	void wallCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct WallGrabTool {
		struct Grabbed : GrabbedWallLikeEntity {
			Grabbed(GrabbedWallLikeEntity grabbed, EditorWallId id, EditorWall grabStartState);
			EditorWallId id;
			EditorWall grabStartState;
		};
		std::optional<Grabbed> grabbed;
	} wallGrabTool;
	void wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	struct LaserCreateTool {
		Vec3 laserColor = EditorLaser::defaultColor.color;
		bool laserPositionLocked = true;
		bool laserRotationLocked = false;
	} laserCreateTool;
	void laserCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	LaserGrabTool laserGrabTool;

	struct MirrorCreateTool {
		std::optional<EditorMirror> update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured);
		void render(GameRenderer& renderer, Vec2 cursorPos);
		void reset();

		EditorMirror makeMirror(Vec2 center, Vec2 cursorPos);

		std::optional<Vec2> center;
		f32 mirrorLength = 0.6f;
		bool mirrorPositionLocked = true;
		EditorMirrorWallType mirrorWallType = EditorMirrorWallType::REFLECTING;
	} mirrorCreateTool;
	void mirrorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct GrabbedOrb {
		Vec2 grabOffset;
	};
	static std::optional<GrabbedOrb> orbCheckGrab(Vec2 position, f32 radius,
		Vec2 cursorPos, bool& cursorCaptured);
	static void grabbedOrbUpdate(Vec2& grabOffset,
		Vec2& position,
		Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	MirrorGrabTool mirrorGrabTool;

	struct TargetCreateTool {
		f32 targetRadius = EditorTarget::defaultRadius;
	} targetCreateTool;
	void targetCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);
	
	struct TargetGrabTool {
		struct Grabbed {
			EditorTargetId id;
			EditorTarget grabStartState;
			Vec2 grabOffset;
		};
		std::optional<Grabbed> grabbed;
	} targetGrabTool;
	void targetGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	void portalCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	PortalGrabTool portalGrabTool;

	void triggerCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct TriggerGrabTool {
		struct Grabbed {
			EditorTriggerId id;
			EditorTrigger grabStartState;
			Vec2 grabOffset;
		};
		std::optional<Grabbed> grabbed;
	} triggerGrabTool;
	void triggerGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	struct DoorCreateTool {
		WallLikeEntityCreateTool tool;
		void render(GameRenderer& renderer, Vec2 cursorPos);

	} doorCreateTool;
	void doorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct DoorGrabTool {
		struct Grabbed : GrabbedWallLikeEntity {
			Grabbed(const GrabbedWallLikeEntity& grabbed, EditorDoorId id, EditorDoor grabStartState);
			EditorDoorId id;
			EditorDoor grabStartState;
		};
		std::optional<Grabbed> grabbed;
	} doorGrabTool;
	void doorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	void modifyLockedCellsUpdate(Vec2 cursorPos, bool& cursorCaptured);
	void modifyLockedCellsToolRender(GameRenderer& renderer, Vec2 cursorPos, bool& cursorCaptured);

	void activateEntity(const EditorEntityId& id);
	void deactivateEntity(const EditorEntityId& id);

	EditorActions actions;
	void freeAction(EditorAction& action);
	void undoAction(const EditorAction& action);
	void redoAction(const EditorAction& action);

	EditorGridTool gridTool;

	void reset();

	bool trySaveLevel(std::string_view path);
	bool tryLoadLevel(std::string_view path);

	struct LevelSaveOpen {
		static constexpr auto SAVE_LEVEL_ERROR_MODAL_NAME = "save level error";
		static void openSaveLevelErrorModal();
		static void saveLevelErrorModal();

		static constexpr auto OPEN_LEVEL_ERROR_MODAL_NAME = "open level error";
		static void openOpenLevelErrorModal();
		static void openLevelErrorModal();

		std::optional<std::string> lastLoadedLevelPath;
	} levelSaveOpen;

	GameState s;
	GameEntities e;
};