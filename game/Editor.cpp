#include "Editor.hpp"
#include <engine/Math/Color.hpp>
#include <game/FileSelectWidget.hpp>
#include <engine/Window.hpp>
#include <engine/Math/Constants.hpp>
#include <engine/Input/Input.hpp>
#include <game/Constants.hpp>
#include <glad/glad.h>
#include <engine/Math/LineSegment.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <engine/Math/Quat.hpp>
#include <game/Stereographic.hpp>

Editor::Editor()
	: actions(EditorActions::make()) {

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

const auto absorbingColor = Color3::WHITE;
const auto reflectingColor = Color3::WHITE / 2.0f;

Vec3 wallColor(EditorWallType type) {
	return type == EditorWallType::REFLECTING
		? reflectingColor
		: absorbingColor;
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
	const auto pointAhead = moveOnStereographicGeodesic(p, tangentAngle, translation);
	return stereographicLine(p, pointAhead);
}

const auto grabbableCircleRadius = 0.015f;

void renderMirror(GameRenderer& renderer, const EditorMirror& mirror) {
	const auto endpoints = mirror.calculateEndpoints();
	renderer.stereographicSegment(endpoints[0], endpoints[1], Color3::WHITE / 2.0f);
	for (const auto endpoint : endpoints) {
		renderer.gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(false));
	}
	renderer.gfx.disk(mirror.center, grabbableCircleRadius, movablePartColor(mirror.positionLocked));
}

void Editor::update(GameRenderer& renderer) {
	auto id = ImGui::DockSpaceOverViewport(
		ImGui::GetMainViewport(),
		ImGuiDockNodeFlags_NoDockingOverCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);

	{
		bool saveButtonDown = false;
		bool saveAsButtonDown = false;
		bool openButtonDown = false;
		bool newButtonDown = false;

		if (ImGui::BeginMainMenuBar()) {
			// Modals can't be opened inside a menu.
			if (ImGui::BeginMenu("level")) {
				if (ImGui::MenuItem("new")) {
					newButtonDown = true;
				}
				if (ImGui::MenuItem("save as")) {
					saveAsButtonDown = true;
				}
				if (ImGui::MenuItem("save")) {
					saveButtonDown = true;
				}
				if (ImGui::MenuItem("open")) {
					openButtonDown = true;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
		std::optional<std::string_view> savePath;

		if (newButtonDown) {
			reset();
			levelSaveOpen.lastLoadedLevelPath = std::nullopt;
		}

		if (Input::isKeyHeld(KeyCode::LEFT_CONTROL) && Input::isKeyDown(KeyCode::S)) {
			saveButtonDown = true;
		}

		if (saveAsButtonDown || (saveButtonDown && !levelSaveOpen.lastLoadedLevelPath.has_value())) {
			savePath = Gui::openFileSave();
		} else if (saveButtonDown && levelSaveOpen.lastLoadedLevelPath.has_value()) {
			savePath = levelSaveOpen.lastLoadedLevelPath;
		}

		if (savePath.has_value()) {
			if (trySaveLevel(*savePath)) {
				levelSaveOpen.lastLoadedLevelPath = savePath;
			} else {
				levelSaveOpen.saveLevelErrorModal();
			}
		}

		if (openButtonDown) {
			const auto path = Gui::openFileSelect();
			if (path.has_value()) {
				if (tryLoadLevel(path->data())) {
					levelSaveOpen.lastLoadedLevelPath = path;
				} else {
					levelSaveOpen.openLevelErrorModal();
				}
			}
		}

		levelSaveOpen.openLevelErrorModal();
		levelSaveOpen.saveLevelErrorModal();
	}

	// Putting this here, because using the dock builder twice (once in simulation once in editor), breaks it (the windows get undocked). 
	// Just using a single id created here doesn't fix anything.
	const auto sideBarWindowName = "settings";
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

		ImGui::Combo("tool", reinterpret_cast<int*>(&selectedTool), "none\0select\0create wall\0create laser\0create mirror\0create target\0create portals\0");

		ImGui::SeparatorText("tool settings");
		switch (selectedTool) {
			using enum Tool;

		case SELECT:
			if (selectTool.selectedEntity == std::nullopt) {
				ImGui::Text("no entity selected");
				break;
			}

			switch (selectTool.selectedEntity->type) {

			case EditorEntityType::WALL: {
				auto entity = walls.get(selectTool.selectedEntity->wall());
				if (!entity.has_value()) {
					break;
				}
				// TODO: Should this add an modify action?
				wallTypeCombo("type", entity->type);
				break;
			}

			case EditorEntityType::LASER: {
				auto laser = lasers.get(selectTool.selectedEntity->laser());
				if (!laser.has_value()) {
					break;
				}
				editorLaserColorCombo("color", laser->color);
				ImGui::Checkbox("position locked", &laser->positionLocked);
				break;
			}

			case EditorEntityType::MIRROR: {
				auto mirror = mirrors.get(selectTool.selectedEntity->mirror());
				if (!mirror.has_value()) {
					break;
				}
				editorMirrorLengthInput(mirror->length);
				ImGui::Checkbox("position locked", &mirror->positionLocked);
				break;
			}

			case EditorEntityType::TARGET: {
				auto target = targets.get(selectTool.selectedEntity->target());
				if (!target.has_value()) {
					break;
				}
				editorTargetRadiusInput(target->radius);
				break;
			}

			case EditorEntityType::PORTAL_PAIR: {
				auto portalPair = portalPairs.get(selectTool.selectedEntity->portalPair());
				if (!portalPair.has_value()) {
					break;
				}
				auto portalGui = [](EditorPortal& portal) {
					ImGui::PushID(&portal);
					ImGui::Combo("wall type", reinterpret_cast<int*>(&portal.wallType), "portal\0reflecting\0absorbing\0");
					ImGui::Checkbox("position locked", &portal.positionLocked);
					ImGui::Checkbox("rotation locked", &portal.rotationLocked);
					ImGui::PopID();
				};

				for (auto& portal : portalPair->portals) {
					portalGui(portal);
				}
				break;
			}

			default:
				break;
			}

			break;

		case WALL:
			wallTypeCombo("type", wallCreateTool.wallType);
			break;

		case LASER:
			editorLaserColorCombo("color", laserCreateTool.laserColor);
			ImGui::Checkbox("position locked", &laserCreateTool.laserPositionLocked);
			break;

		case MIRROR:
			editorMirrorLengthInput(mirrorCreateTool.mirrorLength);
			ImGui::Checkbox("position locked", &mirrorCreateTool.mirrorPositionLocked);
			break;

		case TARGET:
			editorTargetRadiusInput(targetCreateTool.targetRadius);
			break;

		case NONE:
			break;
		default:
			break;
		}

		ImGui::SeparatorText("grid");
		{
			ImGui::Checkbox("show", &showGrid);
			ImGui::TextDisabled("(?)");
			ImGui::SetItemTooltip("Hold ctrl to snap cursor to grid.");

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
		case PORTAL_PAIR: portalCreateToolUpdate(cursorPos, cursorCaptured); break;
		}
	};

	auto grabToolsUpdate = [&] {
		wallGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		laserGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		mirrorGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		targetGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		portalGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
	};

	if (cursorExact) {
		updateSelectedTool();
		grabToolsUpdate();
	} else {
		grabToolsUpdate();
		updateSelectedTool();
	}

	renderer.gfx.camera.aspectRatio = Window::aspectRatio();
	renderer.gfx.camera.zoom = 0.9f;
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, GLsizei(Window::size().x), GLsizei(Window::size().y));

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
	case PORTAL_PAIR: break;
	}

	renderer.gfx.circleTriangulated(Vec2(0.0f), 1.0f, 0.01f, Color3::GREEN);

	const auto boundary = Circle{ Vec2(0.0f), 1.0f };


	for (const auto& wall : walls) {
		renderer.wall(wall->endpoints[0], wall->endpoints[1], wallColor(wall->type));
	}
	renderer.renderWalls();

	for (const auto& mirror : mirrors) {
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

	for (const auto& portalPair : portalPairs) {
		for (const auto& portal : portalPair->portals) {
			const auto endpoints = portal.endpoints();

			const auto normal = Vec2::oriented(portal.normalAngle);
			// Instead of rendering 2 segments could render on that has 2 colors by making a custom triangulation. Then the rendering would need to swap wether the inside or outside of the circle is the side with the normal.
			/*renderer.stereographicSegment(endpoints[0] - normal * 0.007f, endpoints[1] - normal * 0.007, Color3::WHITE);*/
			const auto portalColor = (Color3::RED + Color3::YELLOW) / 2.0f;
			const auto forward = normal * 0.005f;
			const auto back = -normal * 0.005f;
			switch (portal.wallType) {
				using enum EditorPortalWallType;
			case PORTAL:
				renderer.stereographicSegment(endpoints[0], endpoints[1], portalColor);
				break;
			case ABSORBING:
				renderer.stereographicSegment(endpoints[0] + forward, endpoints[1] + forward, portalColor);
				renderer.stereographicSegment(endpoints[0] + back, endpoints[1] + back, absorbingColor);
				break;

			case REFLECTING:
				renderer.stereographicSegment(endpoints[0] + forward, endpoints[1] + forward, portalColor);
				renderer.stereographicSegment(endpoints[0] + back, endpoints[1] + back, reflectingColor);
				break;
			}
			for (const auto endpoint : endpoints) {
				renderer.gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(portal.rotationLocked));
			}
			renderer.gfx.disk(portal.center, grabbableCircleRadius, movablePartColor(portal.positionLocked));
		}
	}

	struct Segment {
		Vec2 endpoints[2];
		Vec3 color; // inefficient
		bool ignore = false;
	};

	std::vector<Segment> laserSegmentsToDraw;

	for (const auto& laser : lasers) {
		renderer.gfx.disk(laser->position, 0.02f, movablePartColor(laser->positionLocked));
		renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), grabbableCircleRadius, movablePartColor(false));

		static i32 maxReflections = 20;
		ImGui::InputInt("max reflections", &maxReflections);

		auto laserPosition = laser->position;
		auto laserDirection = Vec2::oriented(laser->angle);

		struct EditorEntity {
			EditorEntityId id;
			// Used for portals.
			i32 partIndex;
		};
		std::optional<EditorEntity> hitOnLastIteration;
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

			struct Hit {
				Vec2 point;
				f32 distance;
				StereographicLine line;
				EditorEntityId id;
				i32 index;
			};
			std::optional<Hit> closest;
			std::optional<Hit> closestToWrappedAround;

			auto processLineSegmentIntersections = [&laserLine, &laserPosition, &boundaryIntersectionWrappedAround, &laserDirection, &closest, &closestToWrappedAround, &hitOnLastIteration](
				Vec2 endpoint0,
				Vec2 endpoint1,
				EditorEntityId id,
				i32 index = 0) {

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
						&& !(hitOnLastIteration.has_value() && hitOnLastIteration->id == id && hitOnLastIteration->partIndex == index)) {

						if (!closest.has_value() || distance < closest->distance) {
							closest = Hit{ intersection, distance, line, id, index };
						}
					} else {
						if (!closestToWrappedAround.has_value()
							|| distanceToWrappedAround < closestToWrappedAround->distance) {
							closestToWrappedAround = Hit{
								intersection,
								distanceToWrappedAround,
								line,
								id,
								index
							};
						}
					}
				}
			};

			for (const auto& wall : walls) {
				processLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], EditorEntityId(wall.id));
			}

			for (const auto& mirror : mirrors) {
				const auto endpoints = mirror->calculateEndpoints();
				processLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(mirror.id));
			}

			for (const auto& portalPair : portalPairs) {
				for (i32 i = 0; i < 2; i++) {
					const auto& portal = portalPair->portals[i];
					const auto endpoints = portal.endpoints();
					processLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(portalPair.id), i);
				}
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

			/*auto processHit = [&](Vec2 hitPoint, StereographicLine objectHit, const Hit& hit) {*/
			enum HitResult {
				CONTINUE, END
			};

			auto processHit = [&](const Hit& hit) -> HitResult {
				auto laserTangentAtHitPoint = stereographicLineNormalAt(laserLine, hit.point).rotBy90deg();
				if (dot(laserTangentAtHitPoint, laserDirection) > 0.0f) {
					laserTangentAtHitPoint = -laserTangentAtHitPoint;
				}

				auto normalAtHitPoint = stereographicLineNormalAt(hit.line, hit.point);
				if (dot(normalAtHitPoint, laserDirection) > 0.0f) {
					normalAtHitPoint = -normalAtHitPoint;
				}

				auto hitOnNormalSide = [&normalAtHitPoint](f32 hitObjectNormalAngle) {
					return dot(Vec2::oriented(hitObjectNormalAngle), normalAtHitPoint) > 0.0f;
				};

				auto doReflection = [&] {
					laserDirection = laserTangentAtHitPoint.reflectedAroundNormal(normalAtHitPoint);
					laserPosition = hit.point;
					hitOnLastIteration = EditorEntity{ hit.id, hit.index };
					//renderer.gfx.line(laserPosition, laserPosition + laserDirection * 0.2f, 0.01f, Color3::BLUE);
					//renderer.gfx.line(laserPosition, laserPosition + laserTangentAtIntersection * 0.2f, 0.01f, Color3::GREEN);
					//renderer.gfx.line(laserPosition, laserPosition + mirrorNormal * 0.2f, 0.01f, Color3::RED);
				};

				switch (hit.id.type) {
					using enum EditorEntityType;
				case WALL: {
					const auto& wall = walls.get(hit.id.wall());
					if (!wall.has_value()) {
						CHECK_NOT_REACHED();
						return HitResult::END;
					}

					if (wall->type == EditorWallType::ABSORBING) {
						return HitResult::END;
					} else {
						doReflection();
						return HitResult::CONTINUE;
					}
				}

				case MIRROR: {
					//const auto& mirror = mirrors.get(hit.id.mirror());
					doReflection();
					return HitResult::CONTINUE;
				}
				
				case PORTAL_PAIR: {
					const auto portalPair = portalPairs.get(hit.id.portalPair());
					if (!portalPair.has_value()) {
						CHECK_NOT_REACHED();
						return HitResult::END;
					}
					const auto inPortal = portalPair->portals[hit.index];
					const auto hitHappenedOnNormalSide = hitOnNormalSide(inPortal.normalAngle);
					if (!hitHappenedOnNormalSide) {
						switch (inPortal.wallType) {
						case EditorPortalWallType::PORTAL:
							break;
						case EditorPortalWallType::REFLECTING:
							doReflection();
							return HitResult::CONTINUE;
						case EditorPortalWallType::ABSORBING:
							return HitResult::END;
						}
					}
					const auto& inPortalLine = hit.line;
					const auto outPortalIndex = (hit.index + 1) % 2;
					const auto& outPortal = portalPair->portals[outPortalIndex];
					const auto outPortalEndpoints = outPortal.endpoints();
					const auto outPortalLine = stereographicLine(outPortalEndpoints[0], outPortalEndpoints[1]);

					auto dist = stereographicDistance(inPortal.center, hit.point);
					if (dot(hit.point - inPortal.center, Vec2::oriented(inPortal.normalAngle + PI<f32> / 2.0f)) > 0.0f) {
						dist *= -1.0f;
					}
					laserPosition = moveOnStereographicGeodesic(outPortal.center, outPortal.normalAngle + PI<f32> / 2.0f, dist);

					//auto normalAtHitpoint = stereographicLineNormalAt(inPortalLine, hitPoint);

					/*ImGui::Text("%g", normalAtHitPoint.angle());
					renderer.gfx.line(hitPoint, hitPoint + normalAtHitPoint * 0.05f, 0.01f, Color3::GREEN);
					renderer.gfx.line(hitPoint, hitPoint + laserTangentAtIntersection * 0.05f, 0.01f, Color3::BLUE);*/

					//if (dot(normalAtHitPoint, laserDirection) > 0.0f) {
					//	normalAtHitpoint = -normalAtHitpoint;
					//}

					auto outPortalNormalAtOutPoint = stereographicLineNormalAt(outPortalLine, laserPosition);
					if (dot(outPortalNormalAtOutPoint, Vec2::oriented(outPortal.normalAngle)) > 0.0f) {
						outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
					}

					if (dot(normalAtHitPoint, Vec2::oriented(inPortal.normalAngle)) > 0.0f) {
						outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
					}

					// The angle the laser makes with the input mirror normal is the same as the one it makes with the output portal normal.
					auto laserDirectionAngle = 
						laserTangentAtHitPoint.angle() - normalAtHitPoint.angle()
						+ outPortalNormalAtOutPoint.angle();
					laserDirection = Vec2::oriented(laserDirectionAngle);

					hitOnLastIteration = EditorEntity{ hit.id, outPortalIndex };
					return HitResult::CONTINUE;
				}
				
				default:
					break;

				}

				CHECK_NOT_REACHED();
				return HitResult::END;
			};

			if (closest.has_value()) {
				//renderer.stereographicSegmentEx(laserPosition, closest->position, laserColor);
				laserSegmentsToDraw.push_back(Segment{ laserPosition, closest->point, laser->color });
				const auto result = processHit(*closest);
				if (result == HitResult::END) {
					break;
				}
			} else if (closestToWrappedAround.has_value()) {
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersection, laser->color });
				laserSegmentsToDraw.push_back(Segment{ boundaryIntersectionWrappedAround, closestToWrappedAround->point, laser->color });
				/*renderer.gfx.disk(boundaryIntersectionWrappedAround, 0.05f, Color3::RED);
				renderer.gfx.disk(closestToWrappedAround->position, 0.05f, Color3::RED);*/
				const auto result = processHit(*closestToWrappedAround);
				if (result == HitResult::END) {
					break;
				}
			} else {
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersection, laser->color });
				laserSegmentsToDraw.push_back(Segment{ laserPosition, boundaryIntersectionWrappedAround, laser->color });
			}
		}
	}

	for (auto& s : laserSegmentsToDraw) {
		// Lexographically order the endpoints.
		if (s.endpoints[0].x > s.endpoints[1].x 
			|| (s.endpoints[0].x == s.endpoints[1].x && s.endpoints[0].y > s.endpoints[1].y)) {
			std::swap(s.endpoints[0], s.endpoints[1]);
		}
	}

	// This only deals with exacly overlapping segments. There is also the case of partial overlaps that often occurs.
	for (i64 i = 0; i < i64(laserSegmentsToDraw.size()); i++) {
		const auto& a = laserSegmentsToDraw[i];
		for (i64 j = i + 1; j < i64(laserSegmentsToDraw.size()); j++) {
			
			auto& b = laserSegmentsToDraw[j];

			const auto epsilon = 0.01f;
			const auto epsilonSquared = epsilon * epsilon;
			if (a.endpoints[0].distanceSquaredTo(b.endpoints[0]) < epsilonSquared
				&& a.endpoints[1].distanceSquaredTo(b.endpoints[1]) < epsilonSquared) {
				b.ignore = true;
			}
		}
	}
	static bool hideLaser = false;

	//ImGui::Checkbox("hide laser", &hideLaser);

	// TODO: Use additive blending with transaprency maybe.
	i32 drawnSegments = 0;
	if (!hideLaser) {
		for (const auto& segment : laserSegmentsToDraw) {
			if (segment.ignore) {
				continue;
			}
			drawnSegments++;
			renderer.stereographicSegment(segment.endpoints[0], segment.endpoints[1], segment.color);
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

void Editor::reset() {
	walls.reset();
	lasers.reset();
	mirrors.reset();
	targets.reset();
	portalPairs.reset();
	selectTool.selectedEntity = std::nullopt;
	actions.reset();
}

void Editor::mirrorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !mirrorGrabTool.grabbed.has_value()) {
		for (const auto& mirror : mirrors) {
			const auto result = rotatableSegmentCheckGrab(mirror->center, mirror->normalAngle, mirror->length,
				cursorPos, cursorCaptured, cursorExact);

			if (result.has_value()) {
				mirrorGrabTool.grabbed = MirrorGrabTool::Grabbed{
					.id = mirror.id,
					.grabStartState = mirror.entity,
					.grabbed = *result,
				};
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && mirrorGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto mirror = mirrors.get(mirrorGrabTool.grabbed->id);
		if (mirror.has_value()) {
			grabbedRotatableSegmentUpdate(mirrorGrabTool.grabbed->grabbed.grabbedGizmo, mirrorGrabTool.grabbed->grabbed.grabOffset,
				mirror->center, mirror->normalAngle,
				cursorPos, cursorExact);
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
		target.entity = EditorTarget{ .position = cursorPos, .radius = targetCreateTool.targetRadius };
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

void Editor::portalCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		auto portalPair = portalPairs.create();
		const auto offset = Vec2(-0.1f, 0.0f);
		auto makePortal = [&cursorPos](Vec2 offset) {
			return EditorPortal{ .center = cursorPos + offset, .normalAngle = 0.0f, .wallType = EditorPortalWallType::PORTAL, .positionLocked = false, .rotationLocked = false };
		};
		portalPair.entity = EditorPortalPair{
			.portals = { makePortal(offset), makePortal(-offset) }
		};
		actions.add(*this, new EditorActionCreateEntity(EditorEntityId(portalPair.id)));
		cursorCaptured = true;
	}
}

void Editor::portalGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !portalGrabTool.grabbed.has_value()) {
		for (const auto& portalPair : portalPairs) {
			for (i32 i = 0; i < 2; i++) {
				const auto& portal = portalPair->portals[i];

				const auto result = rotatableSegmentCheckGrab(portal.center, portal.normalAngle, EditorPortal::defaultLength,
					cursorPos, cursorCaptured, cursorExact);

				if (result.has_value()) {
					portalGrabTool.grabbed = PortalGrabTool::Grabbed{
						.id = portalPair.id,
						.portalIndex = i,
						.grabStartState = portalPair.entity,
						.grabbed = *result,
					};
				}
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && portalGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto portalPair = portalPairs.get(portalGrabTool.grabbed->id);
		if (portalPair.has_value()) {
			auto& portal = portalPair->portals[portalGrabTool.grabbed->portalIndex];
			grabbedRotatableSegmentUpdate(portalGrabTool.grabbed->grabbed.grabbedGizmo, portalGrabTool.grabbed->grabbed.grabOffset,
				portal.center, portal.normalAngle,
				cursorPos, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && portalGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		auto portalPair = portalPairs.get(portalGrabTool.grabbed->id);
		if (portalPair.has_value()) {
			auto action = new EditorActionModifyPortalPair(
				portalGrabTool.grabbed->id,
				std::move(portalGrabTool.grabbed->grabStartState),
				std::move(*portalPair));
			actions.add(*this, action);
		} else {
			CHECK_NOT_REACHED();
		}
		portalGrabTool.grabbed = std::nullopt;
	}
}

void Editor::freeAction(EditorAction& action) {
	// TODO: Fix entity leaks.
}

#include <iostream>

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
			const auto range = angleRangeBetweenPointsOnCircle(line.circle.center, e0, e1);
			return circularArcDistance(cursorPos, line.circle, range);
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
			const auto dist = stereographicSegmentDistance(endpoints[0], endpoints[1], cursorPos);
			if (dist < Constants::endpointGrabPointRadius) {
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

		for (const auto& portalPair : portalPairs) {
			for (const auto& portal : portalPair->portals) {
				const auto endpoints = portal.endpoints();
				const auto dist = stereographicSegmentDistance(endpoints[0], endpoints[1], cursorPos);
				if (dist < Constants::endpointGrabPointRadius) {
					selectTool.selectedEntity = EditorEntityId(portalPair.id);
					goto selectedEntity;
				}
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

void Editor::activateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;

	case WALL: walls.activate(id.wall()); break;
	case LASER: lasers.activate(id.laser()); break;
	case MIRROR: mirrors.activate(id.mirror()); break;
	case TARGET: targets.activate(id.target()); break;
	case PORTAL_PAIR: portalPairs.activate(id.portalPair()); break;
	}
}

void Editor::deactivateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;
	case WALL: walls.deactivate(id.wall()); break;
	case LASER: lasers.deactivate(id.laser()); break;
	case MIRROR: mirrors.deactivate(id.mirror()); break;
	case TARGET: targets.deactivate(id.target()); break;
	case PORTAL_PAIR: portalPairs.deactivate(id.portalPair()); break;
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
	case MODIFY_PORTAL_PAIR: undoModify(portalPairs, static_cast<const EditorActionModifyPortalPair&>(action)); break;

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
			cursorPos,
			cursorCaptured);
		if (result.has_value()) {
			auto entity = walls.create();
			entity.entity = *result;
			actions.add(*this, new EditorActionCreateEntity(EditorEntityId(entity.id)));
		}
	}
}

void Editor::laserCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (!cursorCaptured && Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto& e = lasers.create();
		e.entity = EditorLaser{ 
			.position = cursorPos, 
			.angle = 0.0f, 
			.color = laserCreateTool.laserColor,
			.positionLocked = laserCreateTool.laserPositionLocked
		};
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
			cursorPos,
			cursorCaptured);
		if (result.has_value()) {
			auto e = mirrors.create();
			e.entity = *result;
			actions.add(*this, new EditorActionCreateEntity(EditorEntityId(e.id)));
		}
	}
}

std::optional<Editor::GrabbedRotatableSegment> Editor::rotatableSegmentCheckGrab(Vec2 center, f32 normalAngle, f32 length,
	Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	
	std::optional<Editor::GrabbedRotatableSegment> grabbed;

	auto updateGrabbed = [&](Vec2 p, RotatableSegmentGizmoType gizmo) {
		if (distance(cursorPos, p) > Constants::endpointGrabPointRadius) {
			return;
		}
		grabbed = GrabbedRotatableSegment{
			.grabOffset = p - cursorPos,
			.grabbedGizmo = gizmo,
		};
		cursorCaptured = true;
	};
	updateGrabbed(center, RotatableSegmentGizmoType::TRANSLATION);
	const auto endpoints = rotatableSegmentEndpoints(center, normalAngle, length);

	if (!cursorCaptured) {
		updateGrabbed(endpoints[0], RotatableSegmentGizmoType::ROTATION_0);
	}

	if (!cursorCaptured) {
		updateGrabbed(endpoints[1], RotatableSegmentGizmoType::ROTATION_1);
	}

	return grabbed;
}

void Editor::grabbedRotatableSegmentUpdate(RotatableSegmentGizmoType type, Vec2 grabOffset,
	Vec2& center, f32& normalAngle,
	Vec2 cursorPos, bool cursorExact) {

	const auto newPosition = cursorPos + grabOffset * !cursorExact;
	switch (type) {
		using enum RotatableSegmentGizmoType;
	case TRANSLATION:
		center = newPosition;
		break;

	case ROTATION_0:
	case ROTATION_1: {
		const auto line = stereographicLine(newPosition, center);

		const auto normalWithCorrectDirection = (newPosition - center).rotBy90deg();
		Vec2 normal(0.0f);
		switch (line.type) {
			using enum StereographicLine::Type;
		case CIRCLE:
			normal = center - line.circle.center;
			break;
		case LINE:
			normal = line.lineNormal;
		}

		// This is done, because stereographicLine changes the center position when the line transitions from CIRCLE to LINE to CIRCLE. This also causes the normal (center - line.circle.center) to change direction, which is visible for example when using portals.
		if (dot(normal, normalWithCorrectDirection) < 0.0f) {
			normal = -normal;
		}

		if (type == ROTATION_1) {
			normal = -normal;
		}

		normalAngle = normal.angle();
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
	case MODIFY_WALL: redoModify(walls, *static_cast<const EditorActionModifyWall*>(&action)); break; 
	case MODIFY_LASER: redoModify(lasers, static_cast<const EditorActionModifyLaser&>(action)); break; 
	case MODIFY_MIRROR: redoModify(mirrors, static_cast<const EditorActionModifyMirror&>(action)); break;
	case MODIFY_TARGET: redoModify(targets, static_cast<const EditorActionModifyTarget&>(action)); break;
	case MODIFY_PORTAL_PAIR: redoModify(portalPairs, static_cast<const EditorActionModifyPortalPair&>(action)); break;

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
 
std::optional<EditorWall> Editor::WallCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured) {
	if (cancelDown) {
		cursorCaptured = true;
		reset();
		return std::nullopt;
	}

	if (!down) {
		return std::nullopt;
	}
	cursorCaptured = true;

	if (!endpoint.has_value()) {
		endpoint = cursorPos;
		return std::nullopt;
	}
	const auto result = EditorWall{ 
		.endpoints = { *endpoint, cursorPos },
		.type = wallType
	};
	reset();
	return result;
}

void Editor::WallCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {
	if (!endpoint.has_value()) {
		return;
	}
	renderer.wall(*endpoint, cursorPos, wallColor(wallType));
}

void Editor::WallCreateTool::reset() {
	endpoint = std::nullopt;
}

std::optional<EditorMirror> Editor::MirrorCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured) {
	if (cancelDown) {
		cursorCaptured = true;
		reset();
		return std::nullopt;
	}

	if (!down) {
		return std::nullopt;
	}
	cursorCaptured = true;

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
	return EditorMirror(center, (cursorPos - center).angle(), mirrorLength, mirrorPositionLocked);
}

void Editor::LevelSaveOpen::openSaveLevelErrorModal() {
	ImGui::OpenPopup(SAVE_LEVEL_ERROR_MODAL_NAME);
}

void Editor::LevelSaveOpen::saveLevelErrorModal() {
	const auto center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal(SAVE_LEVEL_ERROR_MODAL_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}
	ImGui::Text("Failed to save level.");

	if (ImGui::Button("ok")) {
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void Editor::LevelSaveOpen::openOpenLevelErrorModal() {
	ImGui::OpenPopup(OPEN_LEVEL_ERROR_MODAL_NAME);
}

void Editor::LevelSaveOpen::openLevelErrorModal() {
	const auto center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal(OPEN_LEVEL_ERROR_MODAL_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}
	ImGui::Text("Failed to open level.");
	
	if (ImGui::Button("ok")) {
		ImGui::CloseCurrentPopup();
	}
	
	ImGui::EndPopup();
}

