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

void EditorActions::add(EditorAction* action, bool saveSelection) {
    // This is combined into a multiaction, because if the action modifies some entity A and saveSelectedEntityIfModified also modifies entity A then the old modifiction will still have the wrong oldState. This is just the simplest way to not have to deal with that.
    // For example 
    // 1. create laser
    // 2. select laser
    // 3. modify color
    // 4. move laser 
    // move laser triggers saveSelectedEntityIfModified, which saves the laser state to (2, 4) but move laser will save the state from (3, 4). So undoing things wont work. The pairs mean (oldState, newState).
    beginMultiAction();
    if (saveSelection) {
        editor.saveSelectedEntityIfModified();
    }

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
    endMultiAction();
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

EditorActionModifiySelection::EditorActionModifiySelection(std::optional<EditorSelected> oldSelection, std::optional<EditorSelected> newSelection)
    : EditorAction(EditorActionType::MODIFY_SELECTION)
    , oldSelection(oldSelection)
    , newSelection(newSelection) {}

EditorActionModifyLockedCells::EditorActionModifyLockedCells(bool added, i32 index) 
    : EditorAction(EditorActionType::MODIFY_LOCKED_CELLS) 
    , added(added)
    , index(index) {}

EditorSelected::EditorSelected(EditorEntityId id, EditorEntity entityStateAtSelection)
    : id(id)
    , entityStateAtSelection(entityStateAtSelection) {}

const char* editorActionTypeToString(EditorActionType type) {
    switch (type) {
        using enum EditorActionType;
    case MODIFY_WALL: return "modify wall";
    case MODIFY_LASER: return "modify laser";
    case MODIFY_MIRROR: return "modify mirror";
    case MODIFY_TARGET: return "modify target";
    case MODIFY_PORTAL_PAIR: return "modify portal pair";
    case MODIFY_TRIGGER: return "modify trigger";
    case MODIFY_DOOR: return "modify door";
    case CREATE_ENTITY: return "create entity";
    case DESTROY_ENTITY: return "destroy mirror";
    case MODIFY_SELECTION: return "modify selection";
    case MODIFY_LOCKED_CELLS: return "modify locked cells";
    }
    return "";
}
