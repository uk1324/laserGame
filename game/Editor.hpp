#pragma once

#include <game/GameRenderer.hpp>
#include <game/EditorActions.hpp>
#include <game/EditorEntities.hpp>

struct Editor {
	Editor();

	void update(GameRenderer& renderer);
	void undoRedoUpdate();

	enum class Tool {
		NONE,
		SELECT,
		WALL,
		LASER,
		MIRROR,
		TARGET,
		PORTAL_PAIR
	};

	Tool selectedTool = Tool::WALL;

	struct SelectTool {
		std::optional<EditorEntityId> selectedEntity;
	} selectTool;
	void selectToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct WallCreateTool {
		std::optional<EditorWall> update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured);
		void render(GameRenderer& renderer, Vec2 cursorPos);
		void reset();

		std::optional<Vec2> endpoint;

		EditorWallType wallType = EditorWallType::ABSORBING;
	} wallCreateTool;
	void wallCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct WallGrabTool {
		struct Grabbed {
			EditorWallId id;
			i32 index;
			Vec2 offset;
			EditorWall grabStartState;
		};
		std::optional<Grabbed> grabbed;
	} wallGrabTool;
	void wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	struct LaserCreateTool {
		Vec3 laserColor = EditorLaser::defaultColor;
		bool laserPositionLocked = true;
	} laserCreateTool;
	void laserCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

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
	} laserGrabTool;
	void laserGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

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

	enum class RotatableSegmentGizmoType {
		TRANSLATION, ROTATION_0, ROTATION_1
	};
	struct GrabbedRotatableSegment {
		Vec2 grabOffset;
		RotatableSegmentGizmoType grabbedGizmo;
	};
	static std::optional<GrabbedRotatableSegment> rotatableSegmentCheckGrab(Vec2 center, f32 normalAngle, f32 length,
		Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);
	static void grabbedRotatableSegmentUpdate(RotatableSegmentGizmoType type, Vec2 grabOffset,
		Vec2& center, f32& normalAngle,
		Vec2 cursorPos, bool cursorExact);

	struct MirrorGrabTool {
		struct Grabbed {
			EditorMirrorId id;
			EditorMirror grabStartState;
			GrabbedRotatableSegment grabbed;
		};

		std::optional<Grabbed> grabbed;
	} mirrorGrabTool;
	void mirrorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

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

	struct PortalGrabTool {
		struct Grabbed {
			EditorPortalPairId id;
			i32 portalIndex;
			EditorPortalPair grabStartState;
			GrabbedRotatableSegment grabbed;
		};
		std::optional<Grabbed> grabbed;
	} portalGrabTool;
	void portalGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact);

	void activateEntity(const EditorEntityId& id);
	void deactivateEntity(const EditorEntityId& id);

	EditorActions actions;
	void freeAction(EditorAction& action);
	void undoAction(const EditorAction& action);
	void redoAction(const EditorAction& action);

	bool showGrid = true;
	i32 gridLineCount = 16;
	i32 gridCircleCount = 10;

	i32 maxReflections = 30;

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

	EntityArray<EditorWall, EditorWall::DefaultInitialize> walls;
	EntityArray<EditorLaser, EditorLaser::DefaultInitialize> lasers;
	EntityArray<EditorMirror, EditorMirror::DefaultInitialize> mirrors;
	EntityArray<EditorTarget, EditorTarget::DefaultInitialize> targets;
	EntityArray<EditorPortalPair, EditorPortalPair::DefaultInitialize> portalPairs;
};