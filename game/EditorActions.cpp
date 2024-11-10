#include <game/EditorActions.hpp>
#include <game/Editor.hpp>
#include "EditorActions.hpp"

EditorActions EditorActions::make(Editor& editor) {
    return EditorActions{
        .actionStack = List<EditorAction*>::empty(),
        .actions = List<Action>::empty(),
        .editor = editor
    };
}

i64 EditorActions::actionIndexToStackStartOffset(i64 actionIndex) {
    // TODO: This would be already be computed if the offests into the stack were stored instead of sizes. The sizes can be compuated by taking the difference between to positions.
    i64 offset = 0;
    for (i64 i = 0; i < actionIndex; i++) {
        offset += actions[i].subActionCount;
    }
    return offset;
}

void EditorActions::beginMultiAction() {
    if (multiActionNesting > 0) {
        multiActionNesting++;
        return;
    }
    multiActionNesting++;
    currentMultiActionSize = 0;
}

void EditorActions::endMultiAction() {
    ASSERT(multiActionNesting > 0);
    multiActionNesting--;
    if (multiActionNesting > 0) {
        return;
    }
    if (currentMultiActionSize != 0) {
        actions.add(Action{ currentMultiActionSize });
        lastDoneAction++;
    }
}

void EditorActions::add(EditorAction* action) {
    if (lastDoneAction != actions.size() - 1) {
        const auto toFree = actions.size() - 1 - lastDoneAction;
        for (i64 i = 0; i < toFree; i++) {
            const auto& action = actions.back();

            for (i64 j = 0; j < action.subActionCount; j++) {
                auto& actionToDelete = actionStack.back();
                editor.freeAction(*actionToDelete);
                delete actionToDelete;
                actionStack.pop();
            }

            actions.pop();
        }
    }
    if (recordingMultiAction()) {
        currentMultiActionSize++;
    } else {
        lastDoneAction++;
        actions.add(Action(1));
    }
    actionStack.add(std::move(action));
}

bool EditorActions::recordingMultiAction() const {
    return multiActionNesting > 0;
}

void EditorActions::reset() {
    actionStack.clear();
    multiActionNesting = 0;
    currentMultiActionSize = 0;
    actions.clear();
    lastDoneAction = -1;
}

EditorActions::Action::Action(i64 subActionCount)
    : subActionCount(subActionCount) {}

EditorAction::EditorAction(EditorActionType type) 
    : type(type) {}

EditorActionCreateEntity::EditorActionCreateEntity(EditorEntityId id)
    : EditorAction(EditorActionType::CREATE_ENTITY)
    , id(id) {
}

EditorActionDestroyEntity::EditorActionDestroyEntity(EditorEntityId id)
    : EditorAction(EditorActionType::DESTROY_ENTITY)
    , id(id) {
}

EditorActionModifiySelection::EditorActionModifiySelection(std::optional<EditorEntityId> oldSelection, std::optional<EditorEntityId> newSelection) 
    : EditorAction(EditorActionType::MODIFY_SELECTION)
    , oldSelection(oldSelection)
    , newSelection(newSelection) {}
