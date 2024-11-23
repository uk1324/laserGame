#include "Editor.hpp"
#include <engine/Math/Color.hpp>
#include <game/FileSelectWidget.hpp>
#include <engine/Math/Constants.hpp>
#include <engine/Input/Input.hpp>
#include <game/Constants.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <engine/Math/Quat.hpp>
#include <gfx2d/DbgGfx2d.hpp>
#include <game/Stereographic.hpp>
#include <game/GameSerialization.hpp>
#include <game/VectorSet.hpp>
#include <Array2d.hpp>

Editor::Editor()
	: actions(EditorActions::make(*this)) {

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

bool isOrbUnderCursor(Vec2 center, f32 radius, Vec2 cursorPos) {
	const auto circle = stereographicCircle(center, radius);
	const auto d = distance(cursorPos, circle.center);
	return d < circle.radius + Constants::endpointGrabPointRadius;
};

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

		ImGui::Combo("tool", reinterpret_cast<int*>(&selectedTool), 
			"none\0"
			"select\0"
			"create wall\0"
			"create laser\0"
			"create mirror\0"
			"create target\0"
			"create portals\0"
			"create trigger\0"
			"create door\0"
			"modify locked cells\0"
		);

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
				auto entity = e.walls.get(selectTool.selectedEntity->wall());
				if (!entity.has_value()) {
					break;
				}
				// TODO: Should this add an modify action?
				wallTypeCombo("type", entity->type);
				break;
			}

			case EditorEntityType::LASER: {
				auto laser = e.lasers.get(selectTool.selectedEntity->laser());
				if (!laser.has_value()) {
					break;
				}
				editorLaserColorCombo(laser->color);
				ImGui::Checkbox("position locked", &laser->positionLocked);
				break;
			}

			case EditorEntityType::MIRROR: {
				auto mirror = e.mirrors.get(selectTool.selectedEntity->mirror());
				if (!mirror.has_value()) {
					break;
				}
				editorMirrorLengthInput(mirror->length);
				ImGui::Checkbox("position locked", &mirror->positionLocked);
				editorMirrorWallTypeInput(mirror->wallType);
				break;
			}

			case EditorEntityType::TARGET: {
				auto target = e.targets.get(selectTool.selectedEntity->target());
				if (!target.has_value()) {
					break;
				}
				editorTargetRadiusInput(target->radius);
				break;
			}

			case EditorEntityType::PORTAL_PAIR: {
				auto portalPair = e.portalPairs.get(selectTool.selectedEntity->portalPair());
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

			case EditorEntityType::TRIGGER: {
				auto trigger = e.triggers.get(selectTool.selectedEntity->trigger());
				if (!trigger.has_value()) {
					break;
				}
				editorTriggerColorCombo(trigger->color);
				editorTriggerIndexInput("index", trigger->index);
				break;
			}

			case EditorEntityType::DOOR: {
				auto door = e.doors.get(selectTool.selectedEntity->door());
				if (!door.has_value()) {
					break;
				}
				editorTriggerIndexInput("trigger index", door->triggerIndex);
				ImGui::Checkbox("open by default", &door->openByDefault);
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
			editorLaserColorCombo(laserCreateTool.laserColor);
			ImGui::Checkbox("position locked", &laserCreateTool.laserPositionLocked);
			break;

		case MIRROR:
			editorMirrorLengthInput(mirrorCreateTool.mirrorLength);
			ImGui::Checkbox("position locked", &mirrorCreateTool.mirrorPositionLocked);
			editorMirrorWallTypeInput(mirrorCreateTool.mirrorWallType);
			break;

		case TARGET:
			editorTargetRadiusInput(targetCreateTool.targetRadius);
			break;

		case MODIFY_LOCKED_CELLS: {
			bool modified = false;
			ImGui::TextDisabled("(?)");
			ImGui::SetItemTooltip("Add cells with left click remove cells with right click.");

			ImGui::TextDisabled("(!)");
			ImGui::SetItemTooltip("Modifying the values below resets the locked cells.");

			modified |= ImGui::InputInt("segment count", &e.lockedCells.segmentCount);
			e.lockedCells.segmentCount = std::max(e.lockedCells.segmentCount, 0);
			modified |= ImGui::InputInt("ring count", &e.lockedCells.ringCount);
			e.lockedCells.ringCount = std::max(e.lockedCells.ringCount, 0);
			if (modified) {
				e.lockedCells.cells.clear();
			}
			break;
		}
			
		case NONE:
			break;
		default:
			break;
		}

		gridTool.gui();

		ImGui::SeparatorText("debug");
		ImGui::InputInt("max reflections", &s.maxReflections);
		s.maxReflections = std::clamp(s.maxReflections, 0, 100);

		ImGui::End();
	}

	undoRedoUpdate();

	auto cursorPos = Input::cursorPosClipSpace() * renderer.gfx.camera.clipSpaceToWorldSpace();
	bool snappedCursor = false;
	if (Input::isKeyHeld(KeyCode::LEFT_CONTROL)) {
		const auto result = gridTool.snapCursor(cursorPos);
		cursorPos = result.cursorPos;
		snappedCursor = result.snapped;
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
		case TRIGGER: triggerCreateToolUpdate(cursorPos, cursorCaptured); break;
		case DOOR: doorCreateToolUpdate(cursorPos, cursorCaptured); break;
		case MODIFY_LOCKED_CELLS: modifyLockedCellsUpdate(cursorPos, cursorCaptured); break;
		}
	};

	auto grabToolsUpdate = [&] {
		const auto enforceConstraints = false;
		wallGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		laserGrabTool.update(e.lasers, actions, cursorPos, cursorCaptured, cursorExact, enforceConstraints);
		mirrorGrabTool.update(e.mirrors, actions, cursorPos, cursorCaptured, cursorExact, enforceConstraints);
		targetGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		portalGrabTool.update(e.portalPairs, actions, cursorPos, cursorCaptured, cursorExact, enforceConstraints);
		triggerGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
		doorGrabToolUpdate(cursorPos, cursorCaptured, cursorExact);
	};

	if (cursorExact) {
		updateSelectedTool();
		grabToolsUpdate();
	} else {
		grabToolsUpdate();
		updateSelectedTool();
	}

	s.update(e, true);

	renderer.renderClear();

	gridTool.render(renderer);

	switch (selectedTool) {
		using enum Tool;

	case NONE: break;
	case SELECT: break;
	case WALL: wallCreateTool.render(renderer, cursorPos); break;
	case LASER: break;
	case MIRROR: mirrorCreateTool.render(renderer, cursorPos); break;
	case TARGET: break;
	case PORTAL_PAIR: break;
	case TRIGGER: break;
	case DOOR: doorCreateTool.render(renderer, cursorPos); break;
	case MODIFY_LOCKED_CELLS: modifyLockedCellsToolRender(renderer, cursorPos, cursorCaptured);
	}

	renderer.render(e, s, true, true);

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
	e.reset();
	actions.reset();
	selectTool.selectedEntity = std::nullopt;
}

bool Editor::trySaveLevel(std::string_view path) {
	return trySaveGameLevelToFile(e, path);
}

bool Editor::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevelFromFile(e, path);
}

void Editor::targetCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (cursorPos.length() > Constants::boundary.radius) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		auto target = e.targets.create();
		target.entity = EditorTarget{ .position = cursorPos, .radius = targetCreateTool.targetRadius };
		actions.add(new EditorActionCreateEntity(EditorEntityId(target.id)));
		cursorCaptured = true;
	}
}

void Editor::targetGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !targetGrabTool.grabbed.has_value()) {
		for (const auto& target : e.targets) {
			const auto result = orbCheckGrab(target->position, target->radius,
				cursorPos, cursorCaptured);

			if (result.has_value()) {
				targetGrabTool.grabbed = TargetGrabTool::Grabbed{
					.id = target.id,
					.grabStartState = target.entity,
					.grabOffset = result->grabOffset,
				};
				break;
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && targetGrabTool.grabbed.has_value()) {
		auto target = e.targets.get(targetGrabTool.grabbed->id);
		if (target.has_value()) {
			grabbedOrbUpdate(targetGrabTool.grabbed->grabOffset, 
				target->position, 
				cursorPos, cursorCaptured, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}		
	}

	if (isGrabReleased() && targetGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		actions.addModifyAction<EditorTarget, EditorActionModifyTarget>(e.targets, targetGrabTool.grabbed->id, std::move(targetGrabTool.grabbed->grabStartState));
		targetGrabTool.grabbed = std::nullopt;
	}
}

void Editor::portalCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		auto portalPair = e.portalPairs.create();
		const auto offset = Vec2(-0.1f, 0.0f);
		auto makePortal = [&cursorPos](Vec2 offset) {
			return EditorPortal{ .center = cursorPos + offset, .normalAngle = 0.0f, .wallType = EditorPortalWallType::PORTAL, .positionLocked = false, .rotationLocked = false };
		};
		portalPair.entity = EditorPortalPair{
			.portals = { makePortal(offset), makePortal(-offset) }
		};
		actions.add(new EditorActionCreateEntity(EditorEntityId(portalPair.id)));
		cursorCaptured = true;
	}
}

void Editor::triggerCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	if (cursorPos.length() > Constants::boundary.radius) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		auto trigger = e.triggers.create();
		trigger.entity = EditorTrigger{
			.position = cursorPos,
			.color = EditorTrigger::defaultColor.color,
			.index = 0,
		};
		actions.add(new EditorActionCreateEntity(EditorEntityId(trigger.id)));
		cursorCaptured = true;
	}
}

void Editor::triggerGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !triggerGrabTool.grabbed.has_value()) {
		for (const auto& trigger : e.triggers) {
			const auto result = orbCheckGrab(trigger->position, trigger->defaultRadius,
				cursorPos, cursorCaptured);

			if (result.has_value()) {
				triggerGrabTool.grabbed = TriggerGrabTool::Grabbed{
					.id = trigger.id,
					.grabStartState = trigger.entity,
					.grabOffset = result->grabOffset,
				};
				break;
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && triggerGrabTool.grabbed.has_value()) {
		auto trigger = e.triggers.get(triggerGrabTool.grabbed->id);
		if (trigger.has_value()) {
			grabbedOrbUpdate(triggerGrabTool.grabbed->grabOffset,
				trigger->position,
				cursorPos, cursorCaptured, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (isGrabReleased() && triggerGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		actions.addModifyAction<EditorTrigger, EditorActionModifyTrigger>(e.triggers, triggerGrabTool.grabbed->id, std::move(triggerGrabTool.grabbed->grabStartState));
		triggerGrabTool.grabbed = std::nullopt;
	}
}

void Editor::freeAction(EditorAction& action) {
	// TODO: Fix entity leaks.
}

std::optional<Editor::GrabbedWallLikeEntity> Editor::wallLikeEntityCheckGrab(View<const Vec2> endpoints, 
	Vec2 cursorPos, bool& cursorCaptured) {
	
	for (i32 i = 0; i < endpoints.size(); i++) {
		const auto& endpoint = endpoints[i];
		if (distance(cursorPos, endpoint) < Constants::endpointGrabPointRadius) {
			cursorCaptured = true;
			return GrabbedWallLikeEntity{
				.grabbedEndpointIndex = i,
				.grabOffset = endpoint - cursorPos
			};
		}
	}
	return std::nullopt;
}

void Editor::grabbedWallLikeEntityUpdate(const GrabbedWallLikeEntity& grabbed, 
	View<Vec2> endpoints, 
	Vec2 cursorPos, bool cursorExact) {
	endpoints[grabbed.grabbedEndpointIndex] = cursorPos + grabbed.grabOffset * !cursorExact;
}

void Editor::selectToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (Input::isKeyDown(KeyCode::DELETE) && selectTool.selectedEntity.has_value()) {
		deactivateEntity(*selectTool.selectedEntity);
		actions.beginMultiAction();
		{
			actions.add(new EditorActionDestroyEntity(*selectTool.selectedEntity));
			actions.add(new EditorActionModifiySelection(selectTool.selectedEntity, std::nullopt));
			selectTool.selectedEntity = std::nullopt;
		}
		actions.endMultiAction();
	}

	if (cursorCaptured) {
		return;
	}

	const auto oldSelection = selectTool.selectedEntity;

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		for (const auto& wall : e.walls) {
			if (eucledianDistanceToStereographicSegment(wall->endpoints[0], wall->endpoints[1], cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(wall.id);
				goto selectedEntity;
			}
		}

		for (const auto& mirror : e.mirrors) {
			const auto endpoints = mirror->calculateEndpoints();
			const auto dist = eucledianDistanceToStereographicSegment(endpoints[0], endpoints[1], cursorPos);
			if (dist < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(mirror.id);
				goto selectedEntity;
			}
		}

		for (const auto& laser : e.lasers) {
			if (distance(laser->position, cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(laser.id);
				goto selectedEntity;
			}
		}

		for (const auto& target : e.targets) {
			if (isOrbUnderCursor(target->position, target->radius, cursorPos)) {
				selectTool.selectedEntity = EditorEntityId(target.id);
				goto selectedEntity;
			}
		}

		for (const auto& portalPair : e.portalPairs) {
			for (const auto& portal : portalPair->portals) {
				const auto endpoints = portal.endpoints();
				const auto dist = eucledianDistanceToStereographicSegment(endpoints[0], endpoints[1], cursorPos);
				if (dist < Constants::endpointGrabPointRadius) {
					selectTool.selectedEntity = EditorEntityId(portalPair.id);
					goto selectedEntity;
				}
			}
		}

		for (const auto& trigger : e.triggers) {
			const auto circle = trigger->circle();
			if (isOrbUnderCursor(trigger->position, trigger->defaultRadius, cursorPos)) {
				selectTool.selectedEntity = EditorEntityId(trigger.id);
				goto selectedEntity;
			}
		}

		for (const auto& door : e.doors) {
			if (eucledianDistanceToStereographicSegment(door->endpoints[0], door->endpoints[1], cursorPos) < Constants::endpointGrabPointRadius) {
				selectTool.selectedEntity = EditorEntityId(door.id);
				goto selectedEntity;
			}
		}

	} else if (Input::isMouseButtonDown(MouseButton::RIGHT) && selectTool.selectedEntity != std::nullopt) {
		actions.add(new EditorActionModifiySelection(selectTool.selectedEntity, std::nullopt));
		selectTool.selectedEntity = std::nullopt;
	}

	selectedEntity:;
	if (selectTool.selectedEntity != oldSelection) {
		cursorCaptured = true;
		actions.add(new EditorActionModifiySelection(oldSelection, selectTool.selectedEntity));
	}
}

void Editor::doorCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}
	const auto result = doorCreateTool.tool.update(
		Input::isMouseButtonDown(MouseButton::LEFT),
		Input::isMouseButtonDown(MouseButton::RIGHT),
		cursorPos,
		cursorCaptured);

	if (result.has_value()) {
		auto entity = e.doors.create();
		entity.entity = EditorDoor{ 
			.endpoints = { result->endpoints[0], result->endpoints[1] },
			.triggerIndex = 0,
			.openByDefault = false
		};
		actions.add(new EditorActionCreateEntity(EditorEntityId(entity.id)));
	}
}

void Editor::doorGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !doorGrabTool.grabbed.has_value()) {
		for (const auto& door : e.doors) {
			const auto result = wallLikeEntityCheckGrab(constView(door->endpoints),
				cursorPos, cursorCaptured);

			if (result.has_value()) {
				doorGrabTool.grabbed = DoorGrabTool::Grabbed(*result, door.id, door.entity);
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && doorGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		const auto& id = doorGrabTool.grabbed->id;
		auto door = e.doors.get(id);
		if (door.has_value()) {
			grabbedWallLikeEntityUpdate(*doorGrabTool.grabbed, view(door->endpoints), cursorPos, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (isGrabReleased() && doorGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		actions.addModifyAction<EditorDoor, EditorActionModifyDoor>(e.doors, doorGrabTool.grabbed->id, std::move(doorGrabTool.grabbed->grabStartState));
		doorGrabTool.grabbed = std::nullopt;
	}
}

void Editor::modifyLockedCellsUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	const auto index = e.lockedCells.getIndex(cursorPos);

	if (!index.has_value()) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto notDuplicate = addWithoutDuplicating(e.lockedCells.cells, *index);
		if (notDuplicate) {
			actions.add(new EditorActionModifyLockedCells(true, *index));
		}
	}

	if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
		const auto removed = tryRemove(e.lockedCells.cells, *index);
		if (removed) {
			actions.add(new EditorActionModifyLockedCells(false, *index));
		}
	}
}

void Editor::modifyLockedCellsToolRender(GameRenderer& renderer, Vec2 cursorPos, bool& cursorCaptured) {
	if (cursorCaptured) {
		return;
	}

	const auto cellIndex = e.lockedCells.getIndex(cursorPos);
	if (!cellIndex.has_value()) {
		return;
	}
	if (std::ranges::contains(e.lockedCells.cells, cellIndex)) {
		return;
	}
	Vec4 color = GameRenderer::lockedCellColor;
	color.w /= 2.0f;
	renderer.lockedCell(e.lockedCells, *cellIndex, color);
}

void Editor::activateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;

	case WALL: e.walls.activate(id.wall()); break;
	case LASER: e.lasers.activate(id.laser()); break;
	case MIRROR: e.mirrors.activate(id.mirror()); break;
	case TARGET: e.targets.activate(id.target()); break;
	case PORTAL_PAIR: e.portalPairs.activate(id.portalPair()); break;
	case TRIGGER: e.triggers.activate(id.trigger()); break;
	case DOOR: e.doors.activate(id.door()); break;
	}
}

void Editor::deactivateEntity(const EditorEntityId& id) {
	switch (id.type) {
		using enum EditorEntityType;
	case WALL: e.walls.deactivate(id.wall()); break;
	case LASER: e.lasers.deactivate(id.laser()); break;
	case MIRROR: e.mirrors.deactivate(id.mirror()); break;
	case TARGET: e.targets.deactivate(id.target()); break;
	case PORTAL_PAIR: e.portalPairs.deactivate(id.portalPair()); break;
	case TRIGGER: e.triggers.deactivate(id.trigger()); break;
	case DOOR: e.doors.deactivate(id.door()); break;
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
	case MODIFY_WALL: undoModify(e.walls, static_cast<const EditorActionModifyWall&>(action)); break;
	case MODIFY_LASER: undoModify(e.lasers, static_cast<const EditorActionModifyLaser&>(action)); break;
	case MODIFY_MIRROR: undoModify(e.mirrors, static_cast<const EditorActionModifyMirror&>(action)); break;
	case MODIFY_TARGET: undoModify(e.targets, static_cast<const EditorActionModifyTarget&>(action)); break;
	case MODIFY_PORTAL_PAIR: undoModify(e.portalPairs, static_cast<const EditorActionModifyPortalPair&>(action)); break;
	case MODIFY_TRIGGER: undoModify(e.triggers, static_cast<const EditorActionModifyTrigger&>(action)); break;
	case MODIFY_DOOR: undoModify(e.doors, static_cast<const EditorActionModifyDoor&>(action)); break;

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

	case MODIFY_LOCKED_CELLS: {
		const auto& a = static_cast<const EditorActionModifyLockedCells&>(action);
		if (a.added) {
			tryRemove(e.lockedCells.cells, a.index);
		} else {
			addWithoutDuplicating(e.lockedCells.cells, a.index);
		}
		break;
	}

	}
}

void Editor::wallGrabToolUpdate(Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !wallGrabTool.grabbed.has_value()) {
		for (const auto& wall : e.walls) {
			const auto result = wallLikeEntityCheckGrab(constView(wall->endpoints),
				cursorPos, cursorCaptured);

			if (result.has_value()) {
				wallGrabTool.grabbed = WallGrabTool::Grabbed(*result, wall.id, wall.entity);
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && wallGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		const auto& id = wallGrabTool.grabbed->id;
		auto wall = e.walls.get(id);
		if (wall.has_value()) {
			grabbedWallLikeEntityUpdate(*wallGrabTool.grabbed, view(wall->endpoints), cursorPos, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (isGrabReleased() && wallGrabTool.grabbed.has_value()) {
		cursorCaptured = true;
		actions.addModifyAction<EditorWall, EditorActionModifyWall>(e.walls, wallGrabTool.grabbed->id, std::move(wallGrabTool.grabbed->grabStartState));
		wallGrabTool.grabbed = std::nullopt;
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
			auto entity = e.walls.create();
			entity.entity = *result;
			actions.add(new EditorActionCreateEntity(EditorEntityId(entity.id)));
		}
	}
}

void Editor::laserCreateToolUpdate(Vec2 cursorPos, bool& cursorCaptured) {
	if (!cursorCaptured && Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto& ent = e.lasers.create();
		ent.entity = EditorLaser{
			.position = cursorPos, 
			.angle = 0.0f, 
			.color = laserCreateTool.laserColor,
			.positionLocked = laserCreateTool.laserPositionLocked
		};
		actions.add(new EditorActionCreateEntity(EditorEntityId(ent.id)));
		cursorCaptured = true;
		return;
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
			auto ent = e.mirrors.create();
			ent.entity = *result;
			actions.add(new EditorActionCreateEntity(EditorEntityId(ent.id)));
		}
	}
}

std::optional<Editor::GrabbedOrb> Editor::orbCheckGrab(Vec2 position, f32 radius, 
	Vec2 cursorPos, bool& cursorCaptured) {
	if (!isOrbUnderCursor(position, radius, cursorPos)) {
		return std::nullopt;
	}
	cursorCaptured = true;
	return GrabbedOrb{ .grabOffset = position - cursorPos };
}

void Editor::grabbedOrbUpdate(Vec2& grabOffset, 
	Vec2& position, Vec2 cursorPos, 
	bool& cursorCaptured, bool cursorExact) {
	position = cursorPos + grabOffset * !cursorExact;
	cursorCaptured = true;
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
	case MODIFY_WALL: redoModify(e.walls, *static_cast<const EditorActionModifyWall*>(&action)); break; 
	case MODIFY_LASER: redoModify(e.lasers, static_cast<const EditorActionModifyLaser&>(action)); break; 
	case MODIFY_MIRROR: redoModify(e.mirrors, static_cast<const EditorActionModifyMirror&>(action)); break;
	case MODIFY_TARGET: redoModify(e.targets, static_cast<const EditorActionModifyTarget&>(action)); break;
	case MODIFY_PORTAL_PAIR: redoModify(e.portalPairs, static_cast<const EditorActionModifyPortalPair&>(action)); break;
	case MODIFY_TRIGGER: redoModify(e.triggers, static_cast<const EditorActionModifyTrigger&>(action)); break;
	case MODIFY_DOOR: redoModify(e.doors, static_cast<const EditorActionModifyDoor&>(action)); break;

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

	case MODIFY_LOCKED_CELLS: {
		const auto& a = static_cast<const EditorActionModifyLockedCells&>(action);
		if (a.added) {
			addWithoutDuplicating(e.lockedCells.cells, a.index);
		} else {
			tryRemove(e.lockedCells.cells, a.index);
		}
		break;
	}

	}
}
 
std::optional<EditorWall> Editor::WallCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured) {
	const auto wall = tool.update(down, cancelDown, cursorPos, cursorCaptured);
	if (!wall.has_value()) {
		return std::nullopt;
	}
	return EditorWall{ .endpoints = { wall->endpoints[0], wall->endpoints[1] }, .type = wallType };
}

void Editor::WallCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {
	const auto preview = tool.preview(cursorPos);
	if (!preview.has_value()) {
		return;
	}
	renderer.wall(preview->endpoints[0], preview->endpoints[1], GameRenderer::wallColor(wallType));
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
	if (cursorPos.length() > Constants::boundary.radius) {
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
	renderer.mirror(mirror);
}

void Editor::MirrorCreateTool::reset() {
	center = std::nullopt;
}

EditorMirror Editor::MirrorCreateTool::makeMirror(Vec2 center, Vec2 cursorPos) {
	return EditorMirror(center, (cursorPos - center).angle(), mirrorLength, mirrorPositionLocked, mirrorWallType);
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

void Editor::DoorCreateTool::render(GameRenderer& renderer, Vec2 cursorPos) {
	const auto preview = tool.preview(cursorPos);
	if (!preview.has_value()) {
		return;
	}
	renderer.wall(preview->endpoints[0], preview->endpoints[1], GameRenderer::absorbingColor);
}

std::optional<Editor::WallLikeEntity> Editor::WallLikeEntityCreateTool::update(bool down, bool cancelDown, Vec2 cursorPos, bool& cursorCaptured) {
	
	if (cancelDown) {
		cursorCaptured = true;
		reset();
		return std::nullopt;
	}

	if (!down) {
		return std::nullopt;
	}

	if (cursorPos.length() > Constants::boundary.radius) {
		return std::nullopt;
	}

	cursorCaptured = true;

	if (!endpoint.has_value()) {
		endpoint = cursorPos;
		return std::nullopt;
	}
	const auto result = WallLikeEntity{
		.endpoints = { *endpoint, cursorPos },
	};
	reset();
	return result;

}

std::optional<Editor::WallLikeEntity> Editor::WallLikeEntityCreateTool::preview(Vec2 cursorPos) const{
	if (!endpoint.has_value()) {
		return std::nullopt;
	}
	return WallLikeEntity{ .endpoints = { *endpoint, cursorPos } };
}

void Editor::WallLikeEntityCreateTool::reset() {
	endpoint = std::nullopt;
}

Editor::WallGrabTool::Grabbed::Grabbed(GrabbedWallLikeEntity grabbed, EditorWallId id, EditorWall grabStartState)
	: GrabbedWallLikeEntity(grabbed)
	, id(id)
	, grabStartState(grabStartState) {}

Editor::DoorGrabTool::Grabbed::Grabbed(const GrabbedWallLikeEntity& grabbed, EditorDoorId id, EditorDoor grabStartState)
	: GrabbedWallLikeEntity(grabbed)
	, id(id)
	, grabStartState(grabStartState) {}
