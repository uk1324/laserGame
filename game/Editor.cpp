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
#include <engine/Math/LineSegment.hpp>
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

// This reuses code from EditorMirror::calculateEndpoints().
// Could combine them maybe.
StereographicLine stereographicLineThroughPointWithTangent(Vec2 p, f32 tangentAngle, f32 translation = 0.1f) {
	// TODO: Do that same thing in EditorMirror::calculateEndpoints().
	if (p == Vec2(0.0f)) {
		return StereographicLine(Vec2::oriented(tangentAngle + PI<f32> / 2.0f));
	}
	const auto p0 = fromStereographic(p);
	const auto a = p.angle();

	const auto axis = cross(p0, Vec3(0.0f, 0.0f, 1.0f));

	const auto rotateLine = Quat(-tangentAngle + a, p0);
	const auto p1 = rotateLine * (Quat(translation, axis) * p0);
	return stereographicLine(p, toStereographic(p1.normalized()));
}

void renderMirror(GameRenderer& renderer, const EditorMirror& mirror) {
	const auto endpoints = mirror.calculateEndpoints();
	renderer.stereographicSegment(endpoints[0], endpoints[1], Color3::WHITE / 2.0f);
	for (const auto endpoint : endpoints) {
		renderer.gfx.disk(endpoint, 0.01f, Color3::BLUE);
	}
	renderer.gfx.disk(mirror.center, 0.01f, Color3::RED);
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

		ImGui::Combo("tool", reinterpret_cast<int*>(&selectedTool), "none\0select\0create wall\0create laser\0create mirror\0create target\0");

		ImGui::Separator();
		if (selectedTool == Tool::SELECT) {
			if (selectTool.selectedEntity == std::nullopt) {
				ImGui::Text("no entity selected"); 
			}
		}

		ImGui::SeparatorText("grid");
		{
			ImGui::Checkbox("show", &showGrid);
			ImGui::TextDisabled("(?)");
			ImGui::SetItemTooltip("Hold ctrl to enable snapping to grid.");

			ImGui::InputInt("lines", &gridLineCount);
			gridLineCount = std::max(gridLineCount, 1);

			ImGui::InputInt("circles", &gridCircleCount);
			gridCircleCount = std::max(gridCircleCount, 1);
		}

		ImGui::End();
	}

	undoRedoUpdate();

	bool snappedCursor = false;
	auto cursorPos = Input::cursorPosClipSpace() * renderer.gfx.camera.clipSpaceToWorldSpace();

	if (Input::isKeyHeld(KeyCode::LEFT_CONTROL)) {
		const auto snapDistance = Constants::endpointGrabPointRadius;

		for (i32 iLine = 0; iLine < gridLineCount; iLine++) {
			for (i32 iCircle = 0; iCircle < gridCircleCount + 1; iCircle++) {
				auto r = f32(iCircle) / f32(gridCircleCount);
				const auto t = f32(iLine) / f32(gridLineCount);
				const auto a = t * TAU<f32>;
				const auto v = Vec2::fromPolar(a, r);

				if (iCircle == gridCircleCount) {
					r -= 0.01f;
				}

				if (distance(v, cursorPos) < snapDistance) {
					snappedCursor = true;
					cursorPos = v;
					goto exitLoop;
				}
			}
		}
		exitLoop:;

		struct ClosestFeature {
			Vec2 pointOnFeature;
			f32 distance;
		};
		std::optional<ClosestFeature> closestFeature;
		// This could be done without the loop.
		for (i32 iLine = 0; iLine < gridLineCount; iLine++) {
			const auto t = f32(iLine) / f32(gridLineCount);
			const auto a = t * TAU<f32>;
			const auto p = Vec2::oriented(a);

			const auto closestPointOnLine = LineSegment(Vec2(0.0f), p).closestPointTo(cursorPos);
			const auto dist = distance(closestPointOnLine, cursorPos);
			if (!closestFeature.has_value() || dist < closestFeature->distance) {
				closestFeature = ClosestFeature{ .pointOnFeature = closestPointOnLine, .distance = dist };
			}
		}
		// This also could be done without the loop.
		for (i32 iCircle = 1; iCircle < gridCircleCount + 1; iCircle++) {
			const auto r = f32(iCircle) / f32(gridCircleCount);
			const auto pointOnCircle = cursorPos.normalized() * r;
			const auto dist = distance(pointOnCircle, cursorPos);
			if (!closestFeature.has_value() || dist < closestFeature->distance) {
				closestFeature = ClosestFeature{ .pointOnFeature = pointOnCircle, .distance = dist };
			}
		}

		if (!snappedCursor && closestFeature.has_value() && closestFeature->distance < snapDistance) {
			snappedCursor = true;
			cursorPos = closestFeature->pointOnFeature;
		}
	}

	bool cursorCaptured = false;

	// Selection should have higher precedence than grabbing, but creating should have lower precedence than grabbing.
	// The above turns out to not work very well so it got changed. For example when you are trying to place walls with overlapping endpoints. Instead of the wall placing the grab activates.
	// To fix this I give grabbing precedence when cursor isn't snapped.
	if (selectedTool == Tool::SELECT) {
		selectToolUpdate(cursorPos, cursorCaptured);
	}

	// If the cursor was snapped then the user wants to position the cursor exactly in this position. Without cursorExact the offset from the exact position to the grab position is applied to make things more smooth. When this is done it can be very hard to position things exactly on the boundary for example.

	const auto cursorExact = snappedCursor;

	auto updateSelectedTool = [&]() {
		switch (selectedTool) {
			using enum Tool;
		case NONE: break;
		case SELECT: break;
		case WALL: wallCreateToolUpdate(cursorPos, cursorCaptured); break;
		case LASER: laserCreateToolUpdate(cursorPos, cursorCaptured); break;
		case MIRROR: mirrorCreateToolUpdate(cursorPos, cursorCaptured); break;
		case TARGET: targetCreateToolUpdate(cursorPos, cursorCaptured); break;
		}
	};

	auto grabToolsUpdate = [&] {
		wallGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		laserGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		mirrorGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		targetGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
	};

	if (cursorExact) {
		updateSelectedTool();
		grabToolsUpdate();
	} else {
		grabToolsUpdate();
		updateSelectedTool();
	}

	renderer.gfx.camera.aspectRatio = Window::aspectRatio();
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, Window::size().x, Window::size().y);

	const auto gridColor = Color3::WHITE / 15.0f;
	if (showGrid) {
		for (i32 i = 1; i < gridCircleCount; i++) {
			const auto r = f32(i) / f32(gridCircleCount);
			renderer.gfx.circle(Vec2(0.0f), r, 0.01f, gridColor);
		}

		for (i32 i = 0; i < gridLineCount; i++) {
			const auto t = f32(i) / f32(gridLineCount);
			const auto a = t * TAU<f32>;
			const auto v = Vec2::oriented(a);
			renderer.gfx.line(Vec2(0.0f), v, 0.01f, gridColor);
		}
	}
	renderer.gfx.drawLines();
	renderer.gfx.drawCircles();

	switch (selectedTool) {
		using enum Tool;

	case NONE: break;
	case SELECT: break;
	case WALL: wallCreateTool.render(renderer, cursorPos); break;
	case LASER: break;
	case MIRROR: mirrorCreateTool.render(renderer, cursorPos); break;
	case TARGET: break;
	}

	renderer.gfx.circleTriangulated(Vec2(0.0f), 1.0f, 0.01f, Color3::GREEN);

	const auto boundary = Circle{ Vec2(0.0f), 1.0f };

	for (const auto& wall : walls) {
		renderer.wall(wall->endpoints[0], wall->endpoints[1]);
	}
	renderer.renderWalls();

	for (auto mirror : mirrors) {
		renderMirror(renderer, mirror.entity);
	}

	renderer.gfx.drawCircles();
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawDisks();

	for (const auto& target : targets) {
		const auto circle = target->calculateCircle();
		renderer.gfx.circle(circle.center, circle.radius, 0.01f, Color3::MAGENTA);
		renderer.gfx.disk(target->position, 0.01f, Color3::MAGENTA / 2.0f);
	}

	struct Segment {
		Vec2 endpoints[2];
		bool ignore = false;
	};

	std::vector<Segment> laserSegmentsToDraw;

	for (const auto& laser : lasers) {
		renderer.gfx.disk(laser->position, 0.02f, Color3::BLUE);
		renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), 0.01f, Color3::BLUE);


		static i32 maxReflections = 20;
		//ImGui::InputInt("max reflections", &maxReflections);

		auto laserPosition = laser->position;
		auto laserDirection = Vec2::oriented(laser->angle);
		std::optional<EditorEntityId> hitOnLastIteration;
		for (i64 i = 0; i < maxReflections; i++) {
			/*
			The current version is probably better, because it doesn't have an optional return.
			const auto laserCircle = circleThroughPointsWithNormalAngle(laserPosition, laserDirection.angle() + PI<f32> / 2.0f, antipodalPoint(laserPosition));
			const auto laserLine = StereographicLine(*laserCircle);
			*/

			const auto laserLine = stereographicLineThroughPointWithTangent(laserPosition, laserDirection.angle());

			const auto boundaryIntersections = stereographicLineVsCircleIntersection(laserLine, boundary);


			if (boundaryIntersections.size() == 0) {
				// Shouldn't happen if the points are inside, because the antipodal point is always outside.
				// For now do nothing.
				break;
			}
			
			Vec2 boundaryIntersection = boundaryIntersections[0].normalized();
			Vec2 boundaryIntersectionWrappedAround = -boundaryIntersection;

			if (dot(boundaryIntersection - laserPosition, laserDirection) < 0.0f) {
				std::swap(boundaryIntersection, boundaryIntersectionWrappedAround);
			}

			enum class IntersectionType {
				END,
				REFLECT,
			};

			struct Intersection {
				Vec2 position;
				f32 distance;
				StereographicLine objectHit;
				IntersectionType type;
				EditorEntityId id;
			};
			std::optional<Intersection> closest;
			std::optional<Intersection> closestToWrappedAround;

			auto processLineSegmentIntersections = [&laserLine, &laserPosition, &boundaryIntersectionWrappedAround, &laserDirection, &closest, &closestToWrappedAround, &hitOnLastIteration](
				Vec2 endpoint0,
				Vec2 endpoint1,
				IntersectionType type,
				EditorEntityId id) {

				const auto line = stereographicLine(endpoint0, endpoint1);
				const auto intersections = stereographicLineVsStereographicLineIntersection(line, laserLine);

				for (const auto& intersection : intersections) {
					if (const auto outsideBoundary = intersection.length() > 1.0f) {
						continue;
					}

					const auto epsilon = 0.001f;
					const auto lineDirection = (endpoint1 - endpoint0).normalized();
					const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
					const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
					const auto intersectionDAlong = dot(lineDirection, intersection);
					if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
						continue;
					}

					const auto distance = intersection.distanceTo(laserPosition);
					const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);

					if (dot(intersection - laserPosition, laserDirection) > 0.0f
						&& !(hitOnLastIteration.has_value() && hitOnLastIteration == id)) {

						if (!closest.has_value() || distance < closest->distance) {
							closest = Intersection{ intersection, distance, line, type, id };
						}
					} else {
						if (!closestToWrappedAround.has_value()
							|| distanceToWrappedAround < closestToWrappedAround->distance) {
							closestToWrappedAround = Intersection{
								intersection,
								distanceToWrappedAround,
								line,
								type,
								id
							};
						}
					}
				}
			};

			for (const auto& wall : walls) {
				processLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], IntersectionType::END, EditorEntityId(wall.id));
			}

			for (const auto& mirror : mirrors) {
				const auto endpoints = mirror->calculateEndpoints();
				processLineSegmentIntersections(endpoints[0], endpoints[1], IntersectionType::REFLECT, EditorEntityId(mirror.id));
			}

			/*for (const auto& target : targets) {
				const auto circle = target->calculateCircle();
				const auto intersections = stereographicLineVsCircleIntersection(laserLine, circle);

				for (const auto& intersection : intersections) {
					if (const auto outsideBoundary = intersection.length() > 1.0f) {
						continue;
					}

					const auto distance = intersection.distanceTo(laserPosition);
					const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);

					if (dot(intersection - laserPosition, laserDirection) > 0.0f) {
						if (!closest.has_value() || distance < closest->distance) {
							closest = Intersection{ intersection, distance, circle, IntersectionType::END, EditorEntityId(target.id) };
						}
					} else {
						if (!closestToWrappedAround.has_value()
							|| distanceToWrappedAround < closestToWrappedAround->distance) {
							closestToWrappedAround = Intersection{
								intersection,
								distanceToWrappedAround,
								circle,
								IntersectionType::END,
								EditorEntityId(target.id)
							};
						}
					}
				}
			}*/

			auto doReflection = [&](Vec2 hitPoint, StereographicLine objectHit) {
				auto laserTangentAtIntersection = laserLine.type == StereographicLine::Type::CIRCLE
					? (hitPoint - laserLine.circle.center).rotBy90deg().normalized()
					: laserLine.lineNormal.rotBy90deg();

				if (dot(laserTangentAtIntersection, laserDirection) > 0.0f) {
					laserTangentAtIntersection = -laserTangentAtIntersection;
				}
				auto mirrorNormal = objectHit.type == StereographicLine::Type::CIRCLE
					? (hitPoint - objectHit.circle.center).normalized()
					: objectHit.lineNormal;
				if (dot(mirrorNormal, laserDirection) > 0.0f) {
					mirrorNormal = -mirrorNormal;
				}
				laserDirection = laserTangentAtIntersection.reflectedAroundNormal(mirrorNormal);
				laserPosition = hitPoint;

				//renderer.gfx.line(laserPosition, laserPosition + laserDirection * 0.2f, 0.01f, Color3::BLUE);
				//renderer.gfx.line(laserPosition, laserPosition + laserTangentAtIntersection * 0.2f, 0.01f, Color3::GREEN);
				//renderer.gfx.line(laserPosition, laserPosition + mirrorNormal * 0.2f, 0.01f, Color3::RED);
				
			};

			if (closest.has_value()) {
				//renderer.stereographicSegmentEx(laserPosition, closest->position, laserColor);
				laserSegmentsToDraw.push_back(Segment{ laserPosition, closest->position });

				switch (closest->type) {
					using enum IntersectionType;

				case END:
					goto laserEnd;

				case REFLECT: {
					doReflection(closest->position, closest->objectHit);
					hitOnLastIteration = closest->id;
					break;
				}

				}

			} else if (closestToWrappedAround.has_value()) {
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersection });
				laserSegmentsToDraw.push_back(Segment{ boundaryIntersectionWrappedAround, closestToWrappedAround->position });
				/*renderer.gfx.disk(boundaryIntersectionWrappedAround, 0.05f, Color3::RED);
				renderer.gfx.disk(closestToWrappedAround->position, 0.05f, Color3::RED);*/

				switch (closestToWrappedAround->type) {
					using enum IntersectionType;

				case END:
					goto laserEnd;

				case REFLECT:
					doReflection(closestToWrappedAround->position, closestToWrappedAround->objectHit);
					hitOnLastIteration = closestToWrappedAround->id;
					break;
				}
			} else {
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersection });
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersectionWrappedAround });
			}
		}

		laserEnd:;
	}

	for (auto& s : laserSegmentsToDraw) {
		// Lexographically order the endpoints.
		if (s.endpoints[0].x > s.endpoints[1].x 
			|| (s.endpoints[0].x == s.endpoints[1].x && s.endpoints[0].y > s.endpoints[1].y)) {
			std::swap(s.endpoints[0], s.endpoints[1]);
		}
	}

	// This only deals with exacly overlapping segments. There is also the case of partial overlaps that often occurs.
	for (i64 i = 0; i < laserSegmentsToDraw.size(); i++) {
		const auto& a = laserSegmentsToDraw[i];
		for (i64 j = i + 1; j < laserSegmentsToDraw.size(); j++) {
			
			auto& b = laserSegmentsToDraw[j];

			const auto epsilon = 0.01f;
			const auto epsilonSquared = epsilon * epsilon;
			if (a.endpoints[0].distanceSquaredTo(b.endpoints[0]) < epsilonSquared
				&& a.endpoints[1].distanceSquaredTo(b.endpoints[1]) < epsilonSquared) {
				b.ignore = true;
			}
		}
	}
	for (const auto& a : laserSegmentsToDraw) {
		if (a.ignore) {
			continue;
		}

		for (auto& b : laserSegmentsToDraw) {

		}
	}
	static bool hideLaser = false;

	//ImGui::Checkbox("hide laser", &hideLaser);
	i32 drawnSegments = 0;
	if (!hideLaser) {
		for (const auto& segment : laserSegmentsToDraw) {
			if (segment.ignore) {
				continue;
			}
			drawnSegments++;
			renderer.stereographicSegment(segment.endpoints[0], segment.endpoints[1], laserColor);
		}
	}

	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawCircles();
	renderer.gfx.drawDisks();
	renderer.gfx.drawLines();

	if (snappedCursor) {
		renderer.gfx.disk(cursorPos, 0.01f, Color3::WHITE);
	}
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

void Editor::mirrorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !mirrorGrabTool.grabbed.has_value()) {
		for (const auto& mirror : mirrors) {
			if (distance(cursorPos, mirror->center)) {
				auto updateGrabbed = [&](Vec2 p, MirrorGrabTool::GizmoType gizmo) {
					if (distance(cursorPos, p) > Constants::endpointGrabPointRadius) {
						return;
					}
					mirrorGrabTool.grabbed = MirrorGrabTool::Grabbed{
						.id = mirror.id,
						.gizmo = gizmo,
						.grabStartState = mirror.entity,
						.grabOffset = p - cursorPos,
					};
					cursorCaptured = true;
				};
				updateGrabbed(mirror->center, MirrorGrabTool::GizmoType::TRANSLATION);
				const auto endpoints = mirror->calculateEndpoints();
				for (const auto& endpoint : endpoints) {
					if (!cursorCaptured) {
						updateGrabbed(endpoint, MirrorGrabTool::GizmoType::ROTATION);
					}
				}
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && mirrorGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto mirror = mirrors.get(mirrorGrabTool.grabbed->id);
		if (mirror.has_value()) {
			const auto newPosition = cursorPos + mirrorGrabTool.grabbed->grabOffset * !cursorExact;
			switch (mirrorGrabTool.grabbed->gizmo) {
				using enum MirrorGrabTool::GizmoType;
			case TRANSLATION:
				mirror->center = newPosition;
				break;

			case ROTATION:
				mirror->normalAngle = (newPosition - mirror->center).angle() + PI<f32> / 2.0f;
				break;
			}
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && mirrorGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto mirror = mirrors.get(mirrorGrabTool.grabbed->id);
		if (mirror.has_value()) {
			auto action = new EditorActionModifyMirror(
				mirrorGrabTool.grabbed->id,
				std::move(mirrorGrabTool.grabbed->grabStartState),
				std::move(*mirror));
			actions.add(*this, action);
		} else {
			CHECK_NOT_REACHED();
		}
		mirrorGrabTool.grabbed = std::nullopt;
	}
}

void Editor::targetCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		auto target = targets.create();
		target.entity = EditorTarget{ .position = cursorPos };
		actions.add(*this, new EditorActionCreateEntity(EditorEntityId(target.id)));
		cursorCaptured = true;
	}
}

void Editor::targetGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !targetGrabTool.grabbed.has_value()) {
		for (const auto& target : targets) {
			if (distance(target->position, cursorPos) > Constants::endpointGrabPointRadius) {
				continue;
			}
			cursorCaptured = true;
			targetGrabTool.grabbed = TargetGrabTool::Grabbed{
				.id = target.id,
				.grabStartState = target.entity,
				.grabOffset = target->position - cursorPos,
			};
			break;
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && targetGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto target = targets.get(targetGrabTool.grabbed->id);
		if (target.has_value()) {
			target->position = cursorPos + targetGrabTool.grabbed->grabOffset * !cursorExact;
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && targetGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto target = targets.get(targetGrabTool.grabbed->id);
		if (target.has_value()) {
			auto action = new EditorActionModifyTarget(
				targetGrabTool.grabbed->id,
				std::move(targetGrabTool.grabbed->grabStartState),
				std::move(*target));
			actions.add(*this, action);
		} else {
			CHECK_NOT_REACHED();
		}
		targetGrabTool.grabbed = std::nullopt;
	}
}

void Editor::freeAction(EditorAction& action) {
	// TODO: Fix entity leaks.
}

void Editor::selectToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (Input::isKeyDown(KeyCode::DELETE) && selectTool.selectedEntity.has_value()) {
		deactivateEntity(*selectTool.selectedEntity);
		actions.beginMultiAction();
		{
			actions.add(*this, new EditorActionDestroyEntity(*selectTool.selectedEntity));
			actions.add(*this, new EditorActionModifiySelection(selectTool.selectedEntity, std::nullopt));
			selectTool.selectedEntity = std::nullopt;
		}
		actions.endMultiAction();
	}

	if (cursorCaptured) {
		return;
	}

	const auto oldSelection = selectTool.selectedEntity;

	auto stereographicSegmentDistance = [](Vec2 e0, Vec2 e1, Vec2 cursorPos) {
		const auto line = stereographicLine(e0, e1);
		if (line.type == StereographicLine::Type::LINE) {
			return LineSegment(e0, e1).distance(cursorPos);
		} else {
			auto a0 = (e0 - line.circle.center).angle();
			auto a1 = (e1 - line.circle.center).angle();
			if (a0 > a1) {
				std::swap(a0, a1);
			}
			return circularArcDistance(cursorPos, line.circle, a0, a1);
		}
	};

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		for (const auto& wall : walls) {
			if (stereographicSegmentDistance(wall->endpoints[0], wall->endpoints[1], cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(wall.id);
				goto selectedEntity;
			}
		}

		for (const auto& mirror : mirrors) {
			const auto endpoints = mirror->calculateEndpoints();
			if (stereographicSegmentDistance(endpoints[0], endpoints[1], cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(mirror.id);
				goto selectedEntity;
			}
		}

		for (const auto& laser : lasers) {
			if (distance(laser->position, cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(laser.id);
				goto selectedEntity;
			}
		}

		for (const auto& target : targets) {
			const auto circle = target->calculateCircle();
			const auto d = distance(cursorPos, circle.center);
			const auto smallCircle = circle.radius < Constants::endpointGrabPointRadius
				&& d < Constants::endpointGrabPointRadius;

			const auto normalCircle = d < circle.radius;

			if (smallCircle || normalCircle) {
				selectTool.selectedEntity = EditorEntityId(target.id);
				goto selectedEntity;
			}
		}
	} else if (Input::isMouseButtonDown(MouseButton::RIGHT) && selectTool.selectedEntity != std::nullopt) {
		actions.add(*this, new EditorActionModifiySelection(selectTool.selectedEntity, std::nullopt));
		selectTool.selectedEntity = std::nullopt;
	}

	selectedEntity:;
	if (selectTool.selectedEntity != oldSelection) {
		cursorCaptured = true;
		actions.add(*this, new EditorActionModifiySelection(oldSelection, selectTool.selectedEntity));
	}
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

	case TARGET:
		targets.activate(id.target());
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

	case TARGET:
		targets.deactivate(id.target());
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
	case MODIFY_WALL: undoModify(walls, static_cast<const EditorActionModifyWall&>(action)); break; 
	case MODIFY_LASER: undoModify(lasers, static_cast<const EditorActionModifyLaser&>(action)); break;
	case MODIFY_MIRROR: undoModify(mirrors, static_cast<const EditorActionModifyMirror&>(action)); break;
	case MODIFY_TARGET: undoModify(targets, static_cast<const EditorActionModifyTarget&>(action)); break;

	case CREATE_ENTITY: {
		deactivateEntity(static_cast<const EditorActionDestroyEntity&>(action).id);
		break;
	}

	case DESTROY_ENTITY: {
		activateEntity(static_cast<const EditorActionCreateEntity&>(action).id);
		break;
	}

	case MODIFY_SELECTION: {
		const auto& a = static_cast<const EditorActionModifiySelection&>(action);
		selectTool.selectedEntity = a.oldSelection;
		break;
	}
		

	}
}

void Editor::wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
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
				wall->endpoints[wallGrabTool.grabbed->index] = cursorPos + wallGrabTool.grabbed->offset * !cursorExact;
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

void Editor::laserGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
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
			if (!cursorCaptured) {
				updateGrabbed(laser->position, LaserGrabTool::LaserPart::ORIGIN);
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && laserGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto laser = lasers.get(laserGrabTool.grabbed->id);
		if (laser.has_value()) {
			const auto newPosition = cursorPos + laserGrabTool.grabbed->offset * !cursorExact;
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
	case MODIFY_WALL: redoModify(walls, *static_cast<const EditorActionModifyWall*>(&action)); break; 
	case MODIFY_LASER: redoModify(lasers, static_cast<const EditorActionModifyLaser&>(action)); break; 
	case MODIFY_MIRROR: redoModify(mirrors, static_cast<const EditorActionModifyMirror&>(action)); break;
	case MODIFY_TARGET: redoModify(targets, static_cast<const EditorActionModifyTarget&>(action)); break;

	case CREATE_ENTITY: {
		activateEntity(static_cast<const EditorActionCreateEntity&>(action).id);
		break;
	}

	case DESTROY_ENTITY: {
		deactivateEntity(static_cast<const EditorActionDestroyEntity&>(action).id);
		break;
	}

	case MODIFY_SELECTION: {
		const auto& a = static_cast<const EditorActionModifiySelection&>(action);
		selectTool.selectedEntity = a.newSelection;
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

	const auto result = makeMirror(*center, cursorPos);
	reset();
	return result;
}

void Editor::MirrorCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {
	if (!center.has_value()) {
		return;
	}
	const auto mirror = makeMirror(*center, cursorPos);
	renderMirror(renderer, mirror);
}

void Editor::MirrorCreateTool::reset() {
	center = std::nullopt;
}

EditorMirror Editor::MirrorCreateTool::makeMirror(Vec2 center, Vec2 cursorPos) {
	return EditorMirror{ .center = center, .normalAngle = (cursorPos - center).angle() };
}
