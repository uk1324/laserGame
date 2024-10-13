#pragma once

#include <game/GameRenderer.hpp>
#include <game/EditorActions.hpp>
#include <game/EditorEntities.hpp>

struct Editor {
	Editor();

	void update(GameRenderer& renderer);
	void undoRedoUpdate();

	enum class Tool {
		WALL,
		LASER,
		MIRROR,
	};

	Tool selectedTool = Tool::WALL;

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

		std::optional<Vec2> center;
	} mirrorCreateTool;
	void mirrorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured);


	void activateEntity(const EditorEntityId& id);
	void deactivateEntity(const EditorEntityId& id);

	EditorActions actions;
	void freeAction(EditorAction& action);
	void undoAction(const EditorAction& action);
	void redoAction(const EditorAction& action);

	void reset();

	void saveLevel(std::string_view path);
	bool loadLevel(std::string_view path);

	std::optional<std::string> lastLoadedLevel;

	Camera camera;
	EntityArray<EditorWall, EditorWall::DefaultInitialize> walls;
	EntityArray<EditorLaser, EditorLaser::DefaultInitialize> lasers;
	EntityArray<EditorMirror, EditorMirror::DefaultInitialize> mirrors;
};