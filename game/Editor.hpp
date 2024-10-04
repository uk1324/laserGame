#pragma once

#include <game/GameRenderer.hpp>
#include <game/EditorActions.hpp>
#include <game/EditorEntities.hpp>

struct Editor {
	Editor();

	void update(GameRenderer& renderer);

	struct WallCreateTool {
		std::optional<EditorWall> update(bool down, bool cancelDown, Vec2 cursorPos);
		void render(GameRenderer& renderer, Vec2 cursorPos);
		void reset();

		std::optional<Vec2> endpoint;
	} wallCreateTool;

	struct WallGrabTool {
		struct Grabbed {
			EditorWallId id;
			i32 index;
			Vec2 offset;
			EditorWall grabStartState;
		};
		std::optional<Grabbed> grabbed;
	} wallGrabTool;

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
};