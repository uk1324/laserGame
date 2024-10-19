#pragma once

#include <game/GameRenderer.hpp>
#include <game/EditorActions.hpp>
#include <game/EditorEntities.hpp>

struct Editor {
	Editor();

	void update(GameRenderer& renderer);
	void undoRedoUpdate();

	enum class Tool {
		SELECT,
		WALL,
		LASER,
		MIRROR,
		TARGET,
	};

	Tool selectedTool = Tool::WALL;

	struct SelectTool {
		std::optional<EditorEntityId> selectedEntity;
	} selectTool;
	void selectToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct WallCreateTool {
		std::optional<EditorWall> update(bool down, bool cancelDown, Vec2 cursorPos);
		void render(GameRenderer& renderer, Vec2 cursorPos);
		void reset();

		std::optional<Vec2> endpoint;
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
	void wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

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
	void laserGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct MirrorCreateTool {
		std::optional<EditorMirror> update(bool down, bool cancelDown, Vec2 cursorPos);
		void render(GameRenderer& renderer, Vec2 cursorPos);
		void reset();

		static EditorMirror makeMirror(Vec2 center, Vec2 cursorPos);

		std::optional<Vec2> center;
	} mirrorCreateTool;
	void mirrorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	struct MirrorGrabTool {
		enum class GizmoType {
			TRANSLATION, ROTATION
		};

		struct Grabbed {
			EditorMirrorId id;
			GizmoType gizmo;
			EditorMirror grabStartState;
			Vec2 grabOffset;
		};

		std::optional<Grabbed> grabbed;
	} mirrorGrabTool;
	void mirrorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	void targetCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);
	
	struct TargetGrabTool {
		struct Grabbed {
			EditorTargetId id;
			EditorTarget grabStartState;
			Vec2 grabOffset;
		};
		std::optional<Grabbed> grabbed;
	} targetGrabTool;
	void targetGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured);

	void activateEntity(const EditorEntityId& id);
	void deactivateEntity(const EditorEntityId& id);

	EditorActions actions;
	void freeAction(EditorAction& action);
	void undoAction(const EditorAction& action);
	void redoAction(const EditorAction& action);

	void reset();

	void saveLevel(std::string_view path);
	bool loadLevel(std::string_view path);

	bool showGrid = false;
	i32 gridLineCount = 10;
	i32 gridCircleCount = 5;
	bool snapCursorToGrid = false;

	std::optional<std::string> lastLoadedLevel;

	EntityArray<EditorWall, EditorWall::DefaultInitialize> walls;
	EntityArray<EditorLaser, EditorLaser::DefaultInitialize> lasers;
	EntityArray<EditorMirror, EditorMirror::DefaultInitialize> mirrors;
	EntityArray<EditorTarget, EditorTarget::DefaultInitialize> targets;
};