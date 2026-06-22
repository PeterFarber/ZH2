# Phase 4 - Unity-Style Editor

Status: Implemented, with UI/GPU verification gaps.

Source material:

- [x] Read `docs/phase-plans/phase-04-unity-style-editor.md`.
- [x] Read `docs/phase-rules/080-phase-4-unity-style-editor.mdc`.
- [x] Checked `docs/project-structure.md`.
- [x] Checked current source/CMake/test state.

## What This Phase Implements

- [x] `hockey_editor` library and `HockeyMapEditor` app integration.
- [x] Dear ImGui docking editor layer.
- [x] Editor context, settings, theme, menu bar, toolbar, dockspace, and panels.
- [x] Selection, hierarchy, inspector, undo/redo, clipboard, scene workflow, prefab workflow, and project browser.
- [x] Scene/Game viewport rendering, editor camera, transform gizmos, grid, and selection visuals.
- [x] Hockey placement tools.
- [x] Physics preview and gameplay preview.
- [x] Editor tests and headless server guard.

## Finished - Target And Boundaries

- [x] `hockey_editor` target exists.
- [x] `HockeyEditorTests` target exists.
- [x] Editor links core, ECS, renderer, assets, physics, and gameplay.
- [x] Editor owns ImGui, ImGuizmo, native file dialogs, and editor-only UI code.
- [x] Dedicated server remains free of editor UI and ImGui.
- [x] Editor may preview gameplay but does not own gameplay simulation rules.

## Finished - Shell, Layout, Menus, Toolbar

- [x] Editor app and context exist.
- [x] Editor settings persist.
- [x] Editor theme exists.
- [x] Dear ImGui layer exists.
- [x] ImGui Vulkan renderer bridge exists.
- [x] Dockspace and layout persistence exist.
- [x] Main menu bar exists.
- [x] File menu supports scene workflows.
- [x] Edit menu supports undo/redo and selection commands.
- [x] GameObject/Component/Tools/View/Help menu coverage exists.
- [x] Toolbar exists.
- [x] Ctrl+P toggles play mode.

## Finished - Panels

- [x] Panel base and panel manager exist.
- [x] Hierarchy panel exists.
- [x] Inspector panel exists.
- [x] Scene viewport panel exists.
- [x] Game viewport panel exists.
- [x] Project panel exists.
- [x] Console panel exists.
- [x] Stats panel exists.
- [x] Scene validation panel exists.
- [x] Prefab panel exists.
- [x] Project Settings panel exists.
- [x] Project Settings exposes Editor/Client advanced lighting and shadow settings while keeping Server headless settings separate.
- [x] Project Settings navigation scopes repeated page labels to avoid Dear ImGui ID conflicts.
- [x] Project Settings lighting/shadow controls scope repeated tuning labels to avoid Dear ImGui ID conflicts.
- [x] User preferences are exposed from Project Settings.
- [x] Properties panel exists.

## Finished - Editing Workflows

- [x] Selection model exists.
- [x] Multi-selection and primary selection behavior exist.
- [x] Hierarchy context menu behavior exists.
- [x] Focus selected behavior exists.
- [x] Metadata-driven inspector field drawers exist.
- [x] Component add/remove workflow exists.
- [x] Undo/redo command stack exists.
- [x] Clipboard and entity/component snapshot commands exist.
- [x] Dirty state tracking exists.
- [x] New/open/save/save-as scene workflow exists.
- [x] Recent scene and startup scene precedence exist.
- [x] Unsaved changes prompt path exists.
- [x] Autosave exists.
- [x] Scene validation on save/load exists.
- [x] Prefab create, instantiate, drag/drop, apply, and revert workflows exist.

## Finished - Viewports, Gizmos, Tools

- [x] Editor camera controls exist.
- [x] Ground grid exists.
- [x] Transform gizmos exist.
- [x] Selection outline visuals exist.
- [x] Viewport picking exists.
- [x] Full-window screenshot capture exists.
- [x] Scene/Game viewport screenshot capture exists.
- [x] Hockey rink tool exists.
- [x] Hockey spawn tool exists.
- [x] Hockey goal tool exists.
- [x] Hockey puck tool exists.
- [x] Hockey faceoff tool exists.
- [x] Hockey camera rig tool exists.
- [x] Light tool exists.

## Finished - Asset, Physics, Gameplay Integration

- [x] Project browser integrates asset discovery/import/cook workflows.
- [x] Asset drag/drop into editor workflows exists.
- [x] Material and asset reference picker behavior exists.
- [x] Hot reload polling hooks exist.
- [x] Physics preview exists with play/pause/step/reset.
- [x] Collider/trigger viewport visualization exists.
- [x] Gameplay preview snapshots authored scene state.
- [x] Gameplay preview runs fixed gameplay ticks.
- [x] Gameplay preview accepts local preview input.
- [x] Gameplay preview restores authored scene on stop/reset.

## Finished - Tests And Verification

- [x] Editor settings tests exist.
- [x] Selection tests exist.
- [x] Undo/redo tests exist.
- [x] Inspector field tests exist.
- [x] Scene command tests exist.
- [x] Project browser tests exist.
- [x] Hockey tool tests exist.
- [x] Gameplay preview/editor logic tests exist.
- [x] Non-UI editor logic is covered better than panel rendering.

## Started / Partial

- [ ] Toolbar Play/Simulate Pause/Step behavior still needs final runtime UX polish.
- [ ] Panel rendering needs display/GPU verification.
- [ ] Viewport/gizmo/picking behavior needs display/GPU verification.
- [ ] File dialogs need platform/display verification.
- [ ] Game viewport behavior needs display/GPU verification.

## Left To Do

- [ ] Run visual verification on a real GPU/display before treating UI behavior as fully proven.
- [ ] Keep editor-only UI dependencies out of server and non-editor libraries.
