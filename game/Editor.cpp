#include "Editor.hpp"
#include <engine/Math/Color.hpp>
#include <engine/Window.hpp>
#include <engine/Input/Input.hpp>
#include <game/LevelData.hpp>
#include <game/Constants.hpp>
#include <glad/glad.h>
#include <engine/dependencies/Json/JsonPrinter.hpp>
#include <JsonFileIo.hpp>
#include <fstream>
#include <imgui/imgui.h>
#include <game/Level.hpp>

Editor::Editor()
	: actions(EditorActions::make()) {
}

void Editor::update(GameRenderer& renderer) {
	if (Input::isKeyHeld(KeyCode::LEFT_CONTROL)) {

		if (Input::isKeyDown(KeyCode::Z)) {
			if (actions.lastDoneAction >= 0 && actions.lastDoneAction < actions.actions.size()) {
				const auto offset = actions.actionIndexToStackStartOffset(actions.lastDoneAction);
				for (i64 i = offset - 1 + actions.actions[actions.lastDoneAction].subActionCount; i >= offset; i--) {
					undoAction(*actions.actionStack[i]);
				}
				actions.lastDoneAction--;
			}

		} else if (Input::isKeyDown(KeyCode::Y)) {
			if (actions.lastDoneAction >= -1 && actions.lastDoneAction < actions.actions.size() - 1) {
				actions.lastDoneAction++;
				const auto offset = actions.actionIndexToStackStartOffset(actions.lastDoneAction);
				for (i64 i = offset - 1 + actions.actions[actions.lastDoneAction].subActionCount; i >= offset; i--) {
					redoAction(*actions.actionStack[i]);
				}
			}
		}
	}

	const auto cursorPos = Input::cursorPosClipSpace() * camera.clipSpaceToWorldSpace();

	bool cursorCaptured = false;

	if (!cursorCaptured) {
		if (Input::isMouseButtonDown(MouseButton::LEFT) && !wallGrabTool.grabbed.has_value()) {
			for (const auto& wall : walls) {
				for (i32 i = 0; i < std::size(wall->endpoints); i++) {
					const auto& endpoint = wall->endpoints[i];
					if (distance(cursorPos, endpoint) < Constants::wallWidth / 2.0f) {
						cursorCaptured = true;
						wallGrabTool.grabbed = WallGrabTool::Grabbed{
							.id = wall.id,
							.index = i,
							.offset = endpoint - cursorPos,
							.grabStartState = wall.entity,
						};
						goto grabbedWall;
					}
				}
			}
			grabbedWall:;
		}

		if (Input::isMouseButtonHeld(MouseButton::LEFT) && wallGrabTool.grabbed.has_value()) {
			cursorCaptured = true;
			const auto& id = wallGrabTool.grabbed->id;
			auto wall = walls.get(id);
			if (wall.has_value()) {
				wall->endpoints[wallGrabTool.grabbed->index] = cursorPos + wallGrabTool.grabbed->offset;
			} else {
				CHECK_NOT_REACHED();
			}
		}

		if (Input::isMouseButtonUp(MouseButton::LEFT) && wallGrabTool.grabbed.has_value()) {
			cursorCaptured = true;
			const auto& id = wallGrabTool.grabbed->id;
			auto wall = walls.get(id);
			if (wall.has_value()) {
				auto newEntity = *wall;
				auto action = new EditorActionModifyWall(
					id,
					std::move(wallGrabTool.grabbed->grabStartState),
					std::move(newEntity));

				actions.add(*this, action);
			} else {
				CHECK_NOT_REACHED();
			}
			wallGrabTool.grabbed = std::nullopt;
		}
	}

	if (!cursorCaptured) {
		const auto result = wallCreateTool.update(
			Input::isMouseButtonDown(MouseButton::LEFT), 
			Input::isMouseButtonDown(MouseButton::RIGHT),
			cursorPos);
		if (result.has_value()) {
			auto entity = walls.create();
			entity.entity = *result;
			actions.add(*this, new EditorActionCreateEntity(EditorEntityId(entity.id)));
		}
		cursorCaptured = true;
	}

	camera.aspectRatio = Window::aspectRatio();
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	wallCreateTool.render(renderer, cursorPos);

	for (const auto& wall : walls) {
		renderer.wall(wall->endpoints[0], wall->endpoints[1]);
	}

	renderer.renderWalls();
}

void Editor::saveLevel(std::string_view path) {
	auto level = Json::Value::emptyObject();

	{
		auto& jsonWalls = (level[levelWallsName] = Json::Value::emptyArray()).array();
		for (const auto& wall : walls) {
			const auto levelWall = LevelWall{ .e0 = wall->endpoints[0], .e1 = wall->endpoints[1] };
			jsonWalls.push_back(toJson(levelWall));
		}
	}
	std::ofstream file(path.data());
	Json::print(file, level);

}

void Editor::reset() {
	walls.reset();
}

void Editor::freeAction(EditorAction& action) {
}

bool Editor::loadLevel(std::string_view path) {
	const auto jsonOpt = tryLoadJsonFromFile(path);
	if (!jsonOpt.has_value()) {
		return false;
	}
	const auto& json = *jsonOpt;
	try {

		{
			const auto& jsonWalls = json.at(levelWallsName).array();
			for (const auto& jsonWall : jsonWalls) {
				const auto levelWall = fromJson<LevelWall>(jsonWall);
				auto wall = walls.create();
				wall.entity = EditorWall{ .endpoints = { levelWall.e0, levelWall.e1 } };
			}
		}
	}
	catch (const Json::Value::Exception&) {
		return false;
	}

	return true;
}

void Editor::activateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;

	case WALL:
		walls.activate(id.wall());
		break;

	}
}

void Editor::deactivateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;
	case WALL:
		walls.deactivate(id.wall());
		break;
	}
}

template<typename Entity, typename Action>
void undoModify(EntityArray<Entity, typename Entity::DefaultInitialize>& array, const Action& action) {
	auto entity = array.get(action.id);
	if (!entity.has_value()) {
		CHECK_NOT_REACHED();
		return;
	}
	*entity = action.oldEntity;
}

void Editor::undoAction(const EditorAction& action) {
	switch (action.type) {
		using enum EditorActionType;
	case MODIFY_WALL:
		undoModify(walls, static_cast<const EditorActionModifyWall&>(action));
		break;

	case CREATE_ENTITY: {
		deactivateEntity(static_cast<const EditorActionDestroyEntity&>(action).id);
		break;
	}

	case DESTROY_ENTITY: {
		activateEntity(static_cast<const EditorActionCreateEntity&>(action).id);
		break;
	}

	}
}

template<typename Entity, typename Action>
void redoModify(EntityArray<Entity, typename Entity::DefaultInitialize>& array, const Action& action) {
	auto entity = array.get(action.id);
	if (!entity.has_value()) {
		CHECK_NOT_REACHED();
		return;
	}
	*entity = action.newEntity;
}

void Editor::redoAction(const EditorAction& action) {
	switch (action.type) {
		using enum EditorActionType;
	case MODIFY_WALL:
		redoModify(walls, *static_cast<const EditorActionModifyWall*>(&action));
		break;

	case CREATE_ENTITY: {
		activateEntity(static_cast<const EditorActionCreateEntity&>(action).id);
		break;
	}

	case DESTROY_ENTITY: {
		deactivateEntity(static_cast<const EditorActionDestroyEntity&>(action).id);
		break;
	}
	}
}
 
std::optional<EditorWall> Editor::WallCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos) {
	if (cancelDown) {
		reset();
		return std::nullopt;
	}

	if (!down) {
		return std::nullopt;
	}
	if (!endpoint.has_value()) {
		endpoint = cursorPos;
		return std::nullopt;
	}
	const auto result = EditorWall{ .endpoints = { *endpoint, cursorPos } };
	reset();
	return result;
}

void Editor::WallCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {
	if (!endpoint.has_value()) {
		return;
	}
	renderer.wall(*endpoint, cursorPos);
}

void Editor::WallCreateTool::reset() {
	endpoint = std::nullopt;
}
