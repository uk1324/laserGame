#pragma once

#include <game/EditorEntities.hpp>
#include <List.hpp>
#include <unordered_set>

enum class EditorActionType {
	MODIFY_WALL,
	CREATE_ENTITY,
	DESTROY_ENTITY,
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

struct EditorActionCreateEntity : EditorAction {
	EditorActionCreateEntity(EditorEntityId id);

	EditorEntityId id;
};

struct EditorActionDestroyEntity : EditorAction {
	EditorActionDestroyEntity(EditorEntityId id);

	EditorEntityId id;
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
