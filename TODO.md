# Editor Interaction TODO

Status legend:

- [ ] Not started
- [~] In progress
- [x] Done

## Goal

Make common map-editor workflows feel closer to a production scene editor:
multi-object movement should behave as one grouped edit, hierarchy organization
should support direct drag sorting, and major editor controls should explain
themselves through hover tooltips.

## Implementation Order

- [x] 1. Multi-object transform as a group
- [ ] 2. Hierarchy drag sorting and reparent/reorder polish
- [~] 3. Editor-wide tooltip pass
- [~] 4. Focused editor verification and phase-status update

## 1. Multi-Object Transform As A Group

Current state:

- [x] Ctrl multi-select exists in hierarchy and viewport picking.
- [x] Selection outline draws all selected entities.
- [x] Inspector edits the primary entity when multiple entities are selected.
- [x] Transform gizmo Move manipulates selected top-level entities together.

Tasks:

- [x] Add a grouped transform path for multi-selection in the scene viewport.
- [x] Move all selected top-level entities together when the Move gizmo is used.
- [x] Preserve child-relative transforms when both a parent and child are selected.
- [x] Capture before/after transform snapshots so undo/redo restores the full group.
- [x] Keep single-selection behavior unchanged.
- [x] Add editor tests for grouped move and undo/redo.

Acceptance criteria:

- [x] Selecting multiple root objects and dragging the Move gizmo offsets each by
      the same world-space delta.
- [x] Selecting a parent and one of its descendants moves only the parent branch
      once, not the child twice.
- [x] Undo and redo restore every affected entity.
- [x] Existing selection, outline, inspector, and copy/delete behavior still pass.

Likely files:

- `libs/editor/src/Gizmos/TransformGizmo.cpp`
- `libs/editor/include/Hockey/Editor/Gizmos/TransformGizmo.hpp`
- `libs/editor/src/EditorCommands.cpp`
- `libs/editor/include/Hockey/Editor/EditorCommands.hpp`
- `apps/editor_tests/src/UndoRedoTests.cpp`
- `apps/editor_tests/src/SelectionTests.cpp`

## 2. Hierarchy Drag Sorting

Current state:

- [x] Hierarchy supports selection, context menus, inline rename, prefab drops,
      asset drops, and entity drag/drop reparenting.
- [ ] Hierarchy drag/drop currently reparents to a target entity, but does not
      expose explicit sibling ordering.
- [ ] Need to confirm whether root and child order are stable in scene storage
      and serialization before adding UI controls.

Tasks:

- [ ] Identify or add scene APIs for root and sibling ordering.
- [ ] Add editor command support for reorder operations with undo/redo.
- [ ] Support dragging an entity above, below, or into another entity.
- [ ] Prevent invalid drops, including dropping an entity into itself or into one
      of its descendants.
- [ ] Preserve scene dirty-state behavior after reorder operations.
- [ ] Add tests for root reorder, sibling reorder, reparent+order, invalid drops,
      and undo/redo.

Acceptance criteria:

- [ ] Dragging in the hierarchy changes visible order immediately.
- [ ] Saved scenes reload with the same hierarchy order.
- [ ] Undo/redo works for every supported reorder mode.
- [ ] Existing prefab/entity drag-drop behavior remains intact.

Likely files:

- `libs/editor/src/Panels/HierarchyPanel.cpp`
- `libs/editor/include/Hockey/Editor/Panels/HierarchyPanel.hpp`
- `libs/editor/src/EditorCommands.cpp`
- `libs/editor/include/Hockey/Editor/EditorCommands.hpp`
- `libs/ecs` scene/entity hierarchy files, if order APIs are missing
- `apps/editor_tests/src/UndoRedoTests.cpp`
- `apps/editor_tests/src/SceneWorkflowTests.cpp`

## 3. Editor-Wide Tooltips

Current state:

- [x] Some spot tooltips exist in Inspector and Project panels.
- [ ] No consistent tooltip helper or coverage standard exists yet.
- [ ] Main map-editor workflows need hover descriptions for controls, buttons,
      toggles, tool modes, drag/drop targets, and advanced settings.

Tasks:

- [x] Add a small editor UI tooltip helper so tooltip timing and formatting are
      consistent.
- [~] Cover high-traffic panels first: toolbar, hierarchy, inspector, scene
      viewport overlay, project panel, project settings, prefab panel, stats,
      console, and scene validation.
- [~] Add detailed tooltips for transform tools, hockey placement tools,
      lighting/shadow settings, asset import actions, save/open actions, and
      validation issue navigation.
- [ ] Keep tooltip text concise but useful; describe what the control changes,
      not generic UI behavior.
- [ ] Add lightweight contract tests for tooltip helper usage on key panels.

Acceptance criteria:

- [~] Hovering major editor controls shows a useful tooltip.
- [x] Tooltip text does not create duplicate ImGui IDs.
- [x] Helper usage is easy to extend when new panels are added.
- [x] Existing panel rendering and editor tests still pass.

Likely files:

- `libs/editor/include/Hockey/Editor/Ui/Tooltip.hpp`
- `libs/editor/src/Toolbar.cpp`
- `libs/editor/src/MainMenuBar.cpp`
- `libs/editor/src/Panels/HierarchyPanel.cpp`
- `libs/editor/src/Panels/InspectorPanel.cpp`
- `libs/editor/src/Panels/ProjectPanel.cpp`
- `libs/editor/src/Panels/ProjectSettingsPanel.cpp`
- `libs/editor/src/Panels/PrefabPanel.cpp`
- `libs/editor/src/Panels/SceneValidationPanel.cpp`
- `apps/editor_tests/src/ProjectSettingsPanelContractTests.cpp`

## Verification Checklist

- [x] Build editor tests:
      `cmake --build --preset build-windows-debug --target HockeyEditorTests`
- [x] Run editor tests:
      `.\build\windows-debug\apps\editor_tests\HockeyEditorTests.exe --root .`
- [ ] Build map editor:
      `cmake --build --preset build-windows-debug --target HockeyMapEditor`
- [ ] Manually verify in the editor on a real display/GPU:
      multi-select move, hierarchy drag sorting, tooltip hover behavior.
- [~] Update `docs/phase-status/phase-04-unity-style-editor.md` if these items
      change Phase 4 completion or verification state.

## Subagent Work Split

- [x] Agent A: multi-object transform and undo/redo tests.
- [ ] Agent B: hierarchy ordering research and implementation after Agent A if
      it touches shared selection or command code.
- [x] Agent C: tooltip helper and high-traffic panel coverage in parallel when
      its write set does not overlap with active hierarchy work.
- [~] Main agent: integrate subagent changes, resolve overlaps, run verification,
      update phase status, and commit only the intended files.
