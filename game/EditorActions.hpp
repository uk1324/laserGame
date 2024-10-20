#pragma once

#include <game/EditorEntities.hpp>
#include <List.hpp>
#include <unordered_set>

enum class EditorActionType {
	MODIFY_WALL,
	MODIFY_LASER,
	MODIFY_MIRROR,
	MODIFY_TARGET,
	CREATE_ENTITY,
	DESTROY_ENTITY,
	MODIFY_SELECTION,
};

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

struct EditorActionCreateEntity : EditorAction {
	EditorActionCreateEntity(EditorEntityId id);

	EditorEntityId id;
};

struct EditorActionDestroyEntity : EditorAction {
	EditorActionDestroyEntity(EditorEntityId id);

	EditorEntityId id;
};

struct EditorActionModifiySelection : EditorAction {
	EditorActionModifiySelection(std::optional<EditorEntityId> oldSelection, std::optional<EditorEntityId> newSelection);

	std::optional<EditorEntityId> oldSelection;
	std::optional<EditorEntityId> newSelection;
};

struct Editor;

struct EditorActions {
	static EditorActions make();

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

	void add(Editor& editor, EditorAction* action) noexcept;

	i64 multiActionNesting = 0;
	bool recordingMultiAction() const;
	i64 currentMultiActionSize = 0;

	void reset();
};

template<typename T, EditorActionType TYPE>
EditorActionModify<T, TYPE>::EditorActionModify(EntityArrayId<T> id, T&& oldEntity, T&& newEntity)
	: EditorAction(TYPE)
	, id(id)
	, oldEntity(std::move(oldEntity))
	, newEntity(std::move(newEntity)) {}
