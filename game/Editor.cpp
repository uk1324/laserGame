#include "Editor.hpp"
#include <engine/Math/Color.hpp>
#include <engine/Window.hpp>
#include <engine/Math/Constants.hpp>
#include <engine/Input/Input.hpp>
#include <game/LevelData.hpp>
#include <game/Constants.hpp>
#include <glad/glad.h>
#include <engine/dependencies/Json/JsonPrinter.hpp>
#include <JsonFileIo.hpp>
#include <fstream>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <engine/Math/Quat.hpp>
#include <game/Level.hpp>
#include <game/Stereographic.hpp>

const auto laserColor = Color3::CYAN;

Editor::Editor()
	: actions(EditorActions::make()) {

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

Vec2 laserDirectionGrabPoint(const EditorLaser& laser) {
	return laser.position + Vec2::oriented(laser.angle) * 0.15f;
}

void Editor::update(GameRenderer& renderer) {
	auto id = ImGui::DockSpaceOverViewport(
		ImGui::GetMainViewport(),
		ImGuiDockNodeFlags_NoDockingOverCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);

	// Putting this here, because using the dock builder twice (once in simulation once in editor), breaks it (the windows get undocked). 
	// Just using a single id created here doesn't fix anything.
	const auto sideBarWindowName = "sideBar";
	{
		static bool firstFrame = true;
		if (firstFrame) {
			ImGui::DockBuilderRemoveNode(id);
			ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);

			const auto leftId = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.5f, nullptr, &id);
			ImGui::DockBuilderSetNodeSize(leftId, ImVec2(0.2f * ImGui::GetIO().DisplaySize.x, 1.0f));

			ImGui::DockBuilderDockWindow(sideBarWindowName, leftId);

			ImGui::DockBuilderFinish(id);

			firstFrame = false;
		}
	}

	{
		ImGui::Begin(sideBarWindowName);

		ImGui::Combo("tool", reinterpret_cast<int*>(&selectedTool), "wall\0laser\0mirror\0");

		ImGui::End();
	}

	undoRedoUpdate();

	const auto cursorPos = Input::cursorPosClipSpace() * camera.clipSpaceToWorldSpace();
	bool cursorCaptured = false;

	wallGrabToolUpdate(cursorPos, cursorCaptured);
	laserGrabToolUpdate(cursorPos, cursorCaptured);

	switch (selectedTool) {
		using enum Tool;
	case WALL: wallCreateToolUpdate(cursorPos, cursorCaptured); break;
	case LASER: laserCreateToolUpdate(cursorPos, cursorCaptured); break;
	case MIRROR: mirrorCreateToolUpdate(cursorPos, cursorCaptured); break;
	}

	camera.aspectRatio = Window::aspectRatio();
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	wallCreateTool.render(renderer, cursorPos);

	renderer.gfx.circleTriangulated(Vec2(0.0f), 1.0f, 0.01f, Color3::GREEN);

	const auto boundary = Circle{ Vec2(0.0f), 1.0f };

	renderer.gfx.disk(Vec2(0.0f), 0.03f, Color3::GREEN);
	for (const auto& wall : walls) {
		renderer.wall(wall->endpoints[0], wall->endpoints[1]);
	}
	renderer.renderWalls();

	for (auto mirror : mirrors) {
		// The issue with this version is that you cannot specifiy the length easily. Maybe I am missing something idk.

		static f32 b = 0.0f;
		ImGui::SliderFloat("test", &b, 0.0f, TAU<f32>);
		mirror->normalAngle = b;

		const auto p0 = mirror->center;
		const auto p1 = antipodalPoint(p0);
		const auto line = circleThroughPointsWithNormalAngle(p0, mirror->normalAngle, p1);

		if (line.has_value()) {
			//const auto result = circleCircleIntersection(*line, boundary);
			//if (result.has_value()) {
			//	for (const auto& p : *result) {
			//		renderer.gfx.disk(p, 0.03f, Color3::RED);
			//		renderer.gfx.disk(-p, 0.03f, Color3::BLUE);
			//	}
			//}


			renderer.gfx.circle(line->center, line->radius, 0.01f, Color3::YELLOW);
		} else {
			// Shouldn't happen when the points are p and it's antipodal point.
			CHECK_NOT_REACHED();
		}

		//const auto c = fromStereographic(mirror->center);
		//const auto angle = acos(dot(c, Vec3(0.0f, 0.0f, -1.0f)));
		//// shortest rotation that brings c to the bottom of the sphere.
		//const auto axis = cross(c, Vec3(0.0f, 0.0f, 1.0f));
		//const auto coordinateSystemChange = Quat(;

		f32 halfLength = 0.6f;
		const auto c = fromStereographic(mirror->center);
		const auto angle = acos(dot(c, Vec3(0.0f, 0.0f, -1.0f)));
		const auto axis = cross(c, Vec3(0.0f, 0.0f, 1.0f));
		// Because multiplication is left ascocitative this slower than putting the end part in parens.
		const auto a = atan2(mirror->center.y, mirror->center.x);

		

		/*const auto result = Quat(mirror->normalAngle - a, c) * (Quat(halfLength, axis) * c);*/
		const auto result = Quat(-b + a + PI<f32> / 2.0f, c) * (Quat(halfLength, axis) * c);
		const auto p = toStereographic(result);
		const auto l1 = result.length();

		renderer.gfx.disk(p, 0.03f, Color3::RED);
		renderer.gfx.disk(mirror->center, 0.03f, Color3::RED);

		const auto l = stereographicLine(mirror->center, p);
		renderer.gfx.circle(l.center, l.radius, 0.01f, Color3::GREEN);

	}

	renderer.gfx.drawCircles();
	renderer.gfx.drawDisks();

	for (const auto& laser : lasers) {
		renderer.gfx.disk(laser->position, 0.02f, Color3::BLUE);
		renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), 0.01f, Color3::BLUE);
		const auto laserDirection = Vec2::oriented(laser->angle);
		const auto laserLine = stereographicLine(laser->position, laserDirectionGrabPoint(laser.entity));
		//renderer.gfx.circleTriangulated(line.center, line.radius, Constants::wallWidth, Color3::CYAN, 1000);

		const auto boundaryIntersections = circleCircleIntersection(laserLine, boundary);

		if (boundaryIntersections.has_value()) {
			Vec2 boundaryIntersection = (*boundaryIntersections)[0];
			Vec2 boundaryIntersectionWrappedAround = (*boundaryIntersections)[1];

			if (dot(boundaryIntersection - laser->position, laserDirection) < 0.0f) {
				std::swap(boundaryIntersection, boundaryIntersectionWrappedAround);
			}

			for (const auto& wall : walls) {
				const auto wallLine = stereographicLine(wall->endpoints[0], wall->endpoints[1]);
				const auto intersections = circleCircleIntersection(wallLine, laserLine);

				struct Intersection {
					Vec2 position;
					f32 distance;
				};
				if (intersections.has_value()) {
					std::optional<Intersection> closest;
					std::optional<Intersection> closestToWrappedAround;

					for (const auto& intersection : *intersections) {
						if (const auto outsideBoundary = intersection.length() > 1.0f) {
							continue;
						}

						const auto lineDirection = (wall->endpoints[1] - wall->endpoints[0]).normalized();
						const auto dAlong0 = dot(lineDirection, wall->endpoints[0]);
						const auto dAlong1 = dot(lineDirection, wall->endpoints[1]);
						const auto intersectionDAlong = dot(lineDirection, intersection);
						if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
							continue;
						}

						const auto distance = intersection.distanceTo(laser->position);
						const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);

						if (dot(intersection - laser->position, laserDirection) > 0.0f) {
							if (!closest.has_value() || distance < closest->distance) {
								closest = Intersection{ intersection, distance };
							}
						} else {
							if (!closestToWrappedAround.has_value() 
								|| distanceToWrappedAround < closestToWrappedAround->distance) {
								closestToWrappedAround = Intersection{ intersection, distanceToWrappedAround };
							}
						}
					}

					if (closest.has_value()) {
						renderer.stereographicSegment(laser->position, closest->position, laserColor);
					} else if (closestToWrappedAround.has_value()) {
						renderer.stereographicSegment(
							laser->position,
							boundaryIntersection, 
							laserColor);

						renderer.stereographicSegment(
							boundaryIntersectionWrappedAround,
							closestToWrappedAround->position,
							laserColor);
					} else {
						renderer.stereographicSegment(laser->position, boundaryIntersection, laserColor);
						renderer.stereographicSegment(laser->position, boundaryIntersectionWrappedAround, laserColor);
					}
				}
			}

		} else {
			// Shouldn't happen if the points are inside
		}
	}

	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawCircles();
	renderer.gfx.drawDisks();
}

void Editor::undoRedoUpdate() {
	if (Input::isKeyHeld(KeyCode::LEFT_CONTROL)) {
		if (Input::isKeyDownWithAutoRepeat(KeyCode::Z)) {
			if (actions.lastDoneAction >= 0 && actions.lastDoneAction < actions.actions.size()) {
				const auto offset = actions.actionIndexToStackStartOffset(actions.lastDoneAction);
				for (i64 i = offset - 1 + actions.actions[actions.lastDoneAction].subActionCount; i >= offset; i--) {
					undoAction(*actions.actionStack[i]);
				}
				actions.lastDoneAction--;
			}

		} else if (Input::isKeyDownWithAutoRepeat(KeyCode::Y)) {
			if (actions.lastDoneAction >= -1 && actions.lastDoneAction < actions.actions.size() - 1) {
				actions.lastDoneAction++;
				const auto offset = actions.actionIndexToStackStartOffset(actions.lastDoneAction);
				for (i64 i = offset - 1 + actions.actions[actions.lastDoneAction].subActionCount; i >= offset; i--) {
					redoAction(*actions.actionStack[i]);
				}
			}
		}
	}
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
	// TODO: Fix entity leaks.
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

	case LASER:
		lasers.activate(id.laser());
		break;

	case MIRROR:
		mirrors.activate(id.mirror());
		break;

	}
}

void Editor::deactivateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;
	case WALL:
		walls.deactivate(id.wall());
		break;

	case LASER:
		lasers.deactivate(id.laser());
		break;

	case MIRROR:
		mirrors.deactivate(id.mirror());
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

	case MODIFY_LASER:
		undoModify(lasers, static_cast<const EditorActionModifyLaser&>(action));
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

void Editor::wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (!cursorCaptured) {
		if (Input::isMouseButtonDown(MouseButton::LEFT) && !wallGrabTool.grabbed.has_value()) {
			for (const auto& wall : walls) {
				for (i32 i = 0; i < std::size(wall->endpoints); i++) {
					const auto& endpoint = wall->endpoints[i];
					if (distance(cursorPos, endpoint) < Constants::endpointGrabPointRadius) {
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
}

void Editor::wallCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
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
}

void Editor::laserCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (!cursorCaptured && Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto& e = lasers.create();
		e.entity = EditorLaser{ .position = cursorPos, .angle = 0.0f };
		actions.add(*this, new EditorActionCreateEntity(EditorEntityId(e.id)));
		cursorCaptured = true;
		return;
	}
}

void Editor::laserGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !laserGrabTool.grabbed.has_value()) {
		for (const auto& laser : lasers) {
			auto updateGrabbed = [&](Vec2 p, LaserGrabTool::LaserPart part) {
				if (distance(cursorPos, p) > Constants::endpointGrabPointRadius) {
					return;
				}
				laserGrabTool.grabbed = LaserGrabTool::Grabbed{
					.id = laser.id,
					.part = part,
					.grabStartState = laser.entity,
					.offset = p - cursorPos,
				};
				cursorCaptured = true;
			};

			updateGrabbed(laserDirectionGrabPoint(laser.entity), LaserGrabTool::LaserPart::DIRECTION);
			updateGrabbed(laser->position, LaserGrabTool::LaserPart::ORIGIN);
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && laserGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto laser = lasers.get(laserGrabTool.grabbed->id);
		if (laser.has_value()) {
			const auto newPosition = cursorPos + laserGrabTool.grabbed->offset;
			switch (laserGrabTool.grabbed->part) {
				using enum LaserGrabTool::LaserPart;

			case ORIGIN:
				laser->position = newPosition;
				break;
			case DIRECTION:
				laser->angle = (newPosition - laser->position).angle();
				break;
			}
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && laserGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto laser = lasers.get(laserGrabTool.grabbed->id);
		if (laser.has_value()) {
			auto action = new EditorActionModifyLaser(
				laserGrabTool.grabbed->id,
				std::move(laserGrabTool.grabbed->grabStartState),
				std::move(*laser));
			actions.add(*this, action);
		} else {
			CHECK_NOT_REACHED();
		}
		laserGrabTool.grabbed = std::nullopt;
	}
}

void Editor::mirrorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (!cursorCaptured) {
		const auto result = mirrorCreateTool.update(
			Input::isMouseButtonDown(MouseButton::LEFT),
			Input::isMouseButtonDown(MouseButton::RIGHT),
			cursorPos);
		if (result.has_value()) {
			auto e = mirrors.create();
			e.entity = *result;
			actions.add(*this, new EditorActionCreateEntity(EditorEntityId(e.id)));
		}
		cursorCaptured = true;
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

	case MODIFY_LASER:
		redoModify(lasers, static_cast<const EditorActionModifyLaser&>(action));
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

std::optional<EditorMirror> Editor::MirrorCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos) {
	if (cancelDown) {
		reset();
		return std::nullopt;
	}

	if (!down) {
		return std::nullopt;
	}

	if (!center.has_value()) {
		center = cursorPos;
		return std::nullopt;
	}

	const auto result = EditorMirror{ .center = *center, .normalAngle = (cursorPos - *center).angle() };
	reset();
	return result;
}

void Editor::MirrorCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {

}

void Editor::MirrorCreateTool::reset() {
	center = std::nullopt;
}
