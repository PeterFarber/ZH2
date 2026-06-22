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
- [x] 2. Hierarchy drag sorting and reparent/reorder polish
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
- [x] Hierarchy drag/drop supports before, inside, and after row drop placement.
- [x] Hierarchy drag/drop draws distinct previews for sibling insertion versus
      parent/inside drops.
- [x] Root and child order are stable in scene storage and scene serialization.

Tasks:

- [x] Identify or add scene APIs for root and sibling ordering.
- [x] Add editor command support for reorder operations with undo/redo.
- [x] Support dragging an entity above, below, or into another entity.
- [x] Prevent invalid drops, including dropping an entity into itself or into one
      of its descendants.
- [x] Preserve scene dirty-state behavior after reorder operations.
- [x] Add tests for root reorder, sibling reorder, reparent+order, invalid drops,
      and undo/redo.
- [x] Add custom drag/drop preview drawing so moving an entity does not look like
      selecting or focusing a parent row.

Acceptance criteria:

- [~] Dragging in the hierarchy changes visible order immediately; code path is
      implemented and still needs real display/GPU verification.
- [~] Hovering a drag target shows a distinct insertion line for reorder and a
      filled/outlined row for parent drops; code path is implemented and still
      needs real display/GPU verification.
- [x] Saved scenes reload with the same hierarchy order.
- [x] Undo/redo works for every supported reorder mode.
- [x] Existing prefab/entity drag-drop behavior remains intact in covered tests.

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

- [x] Shared tooltip helper exists and defines timing/disabled-control behavior.
- [x] First-pass tooltip coverage exists for high-traffic editor panels and controls.
- [x] Project Settings renderer, lighting/shadow, physics, gameplay, startup
      scene, asset, and preference tooltips describe scene impact.
- [~] Some lower-traffic workflow controls may still need follow-up hover text
      during live UI review.

Tasks:

- [x] Add a small editor UI tooltip helper so tooltip timing and formatting are
      consistent.
- [x] Cover high-traffic panels first: toolbar, hierarchy, inspector, scene
      viewport overlay, project panel, project settings, prefab panel, stats,
      console, and scene validation.
- [~] Add detailed tooltips for transform tools, hockey placement tools,
      lighting/shadow settings, asset import actions, save/open actions, and
      validation issue navigation.
  - [x] Project Settings graphics, lighting/shadow, physics, gameplay, startup
        scene, asset, and preference controls have scene-impact tooltip copy.
- [x] Keep tooltip text concise but useful; describe what the control changes,
      not generic UI behavior.
- [x] Add lightweight contract tests for tooltip helper usage on key panels.

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

## Work Notes

- [x] Multi-object transform and undo/redo tests were completed first.
- [x] Hierarchy ordering was implemented manually in this pass; no subagents are
      part of the current workflow.
- [x] Tooltip helper and first-pass panel coverage are already in place.
- [~] Finish focused verification, update phase status, and commit only the
      intended hierarchy/order changes.
