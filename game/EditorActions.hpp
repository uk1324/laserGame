#pragma once

#include <game/EditorEntities.hpp>
#include <List.hpp>
#include <unordered_set>

enum class EditorActionType {
	MODIFY_WALL,
	MODIFY_LASER,
	MODIFY_MIRROR,
	MODIFY_TARGET,
	MODIFY_PORTAL_PAIR,
	MODIFY_TRIGGER,
	MODIFY_DOOR,
	CREATE_ENTITY,
	DESTROY_ENTITY,
	MODIFY_SELECTION,
	MODIFY_LOCKED_CELLS
};

const char* editorActionTypeToString(EditorActionType type);

struct EditorAction {
	EditorAction(EditorActionType type);

	EditorActionType type;
};

template<typename T, EditorActionType TYPE>
struct EditorActionModify : EditorAction {
	EditorActionModify(EntityArrayId<T> id, T&& oldEntity, T&& newEntity);

	EntityArrayId<T> id;
	T oldEntity, newEntity;
};
using EditorActionModifyWall = EditorActionModify<EditorWall, EditorActionType::MODIFY_WALL>;
using EditorActionModifyLaser = EditorActionModify<EditorLaser, EditorActionType::MODIFY_LASER>;
using EditorActionModifyMirror = EditorActionModify<EditorMirror, EditorActionType::MODIFY_MIRROR>;
using EditorActionModifyTarget = EditorActionModify<EditorTarget, EditorActionType::MODIFY_TARGET>;
using EditorActionModifyPortalPair = EditorActionModify<EditorPortalPair, EditorActionType::MODIFY_PORTAL_PAIR>;
using EditorActionModifyTrigger = EditorActionModify<EditorTrigger, EditorActionType::MODIFY_TRIGGER>;
using EditorActionModifyDoor = EditorActionModify<EditorDoor, EditorActionType::MODIFY_DOOR>;

struct EditorActionCreateEntity : EditorAction {
	EditorActionCreateEntity(EditorEntityId id);

	EditorEntityId id;
};

struct EditorActionDestroyEntity : EditorAction {
	EditorActionDestroyEntity(EditorEntityId id);

	EditorEntityId id;
};

struct EditorSelected {
	EditorSelected(EditorEntityId id, EditorEntity entityStateAtSelection);

	EditorEntityId id;
	EditorEntity entityStateAtSelection;
};

struct EditorActionModifiySelection : EditorAction {
	EditorActionModifiySelection(std::optional<EditorSelected> oldSelection, std::optional<EditorSelected> newSelection);

	std::optional<EditorSelected> oldSelection;
	std::optional<EditorSelected> newSelection;
};

struct EditorActionModifyLockedCells : EditorAction {
	// added or removed
	EditorActionModifyLockedCells(bool added, i32 index);

	bool added;
	i32 index;
};

struct Editor;

struct EditorActions {
	static EditorActions make(Editor& editor);

	List<EditorAction*> actionStack;
	// Multiactions can contain multiple actions.
	struct Action {
		Action(i64 subActionCount);
		i64 subActionCount;
	};
	List<Action> actions;
	// When every action is undone lastDoneAction = -1.
	i64 lastDoneAction = -1;

	i64 actionIndexToStackStartOffset(i64 actionIndex);

	// returns if a multicommand actually started.
	void beginMultiAction();
	void endMultiAction();

	void add(EditorAction* action, bool saveSelection = true);
	template<typename Entity, typename ModifyAction>
	void addModifyAction(
		EntityArray<Entity, typename Entity::DefaultInitialize>& entities,
		EntityArrayId<Entity> id,
		Entity&& oldState);

	i64 multiActionNesting = 0;
	bool recordingMultiAction() const;
	i64 currentMultiActionSize = 0;

	void reset();

	Editor& editor;
};

template<typename T, EditorActionType TYPE>
EditorActionModify<T, TYPE>::EditorActionModify(EntityArrayId<T> id, T&& oldEntity, T&& newEntity)
	: EditorAction(TYPE)
	, id(id)
	, oldEntity(std::move(oldEntity))
	, newEntity(std::move(newEntity)) {}

template<typename Entity, typename ModifyAction>
void EditorActions::addModifyAction(
	EntityArray<Entity, typename Entity::DefaultInitialize>& entities,
	EntityArrayId<Entity> id,
	Entity&& oldState) {

	auto entity = entities.get(id);

	if (!entity.has_value()) {
		CHECK_NOT_REACHED();
		return;
	}
	auto action = new ModifyAction(id, std::move(oldState), std::move(*entity));
	add(action);
}
