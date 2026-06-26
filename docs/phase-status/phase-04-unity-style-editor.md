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
- [x] Shared editor tooltip helper exists and covers first-pass Toolbar, Hierarchy, Inspector, Project Settings, Project Panel, Scene View Overlay, Prefab, Stats, Console, and Scene Validation controls.
- [x] Shared Font Awesome editor icon font/helper exists and covers first-pass Toolbar, Main Menu, Project Panel, Hierarchy, Inspector, asset inspector, field drawers, and Scene View Overlay controls.
- [x] Project Settings tooltips describe scene impact for renderer, lighting/shadow, physics, gameplay, startup scene, asset, and preference controls.
- [x] Gameplay Tuning panel exposes runtime gameplay settings and YAML tuning values.
- [x] Project Settings and Gameplay Tuning expose waypoint marker prefab selection with prefab drag/drop.
- [x] User preferences are exposed from Project Settings.
- [x] Properties panel exists.

## Finished - Editing Workflows

- [x] Selection model exists.
- [x] Multi-selection and primary selection behavior exist.
- [x] Move gizmo translates selected top-level entities as one grouped undoable edit.
- [x] Hierarchy context menu behavior exists.
- [x] Hierarchy drag sorting supports root/sibling reorder, before/inside/after drops, and undo/redo.
- [x] Hierarchy drag sorting uses distinct custom previews for sibling insertion versus parent/inside drops.
- [x] Focus selected behavior exists.
- [x] Metadata-driven inspector field drawers exist.
- [x] Rigid Body collision detection mode is exposed through metadata-driven Inspector enum fields.
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
- [x] Scene View draws selectable 2D icons for meshless hockey marker components.
- [x] Full-window screenshot capture exists.
- [x] Scene/Game viewport screenshot capture exists.
- [x] Hockey rink tool exists.
- [x] Hockey spawn tool creates team-owned normal `SpawnPointComponent` markers without role/index fields.
- [x] Hockey goal tool exists.
- [x] Hockey puck tool exists.
- [x] Hockey puck tool authors dynamic puck bodies with continuous collision detection.
- [x] Hockey faceoff tool creates neutral, Home-penalty, and Away-penalty `SpawnPointComponent` faceoff pools.
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
- [x] Hierarchy reorder command and panel wiring tests exist.
- [x] Hierarchy drop-preview panel wiring tests exist.
- [x] Editor tooltip helper contract tests cover high-traffic panels and Project Settings scene-impact tooltip copy.
- [x] Editor icon font/helper contract tests cover vendored assets, ImGui font loading, and first-pass high-traffic icon adoption.
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
