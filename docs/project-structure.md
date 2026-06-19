# HockeyGame Project Structure

This document is the repository map for HockeyGame. It explains where code and
data live, how CMake targets relate to each other, and which scripts are used for
common workflows.

It intentionally does not track phase progress or implementation checklists.
Use [docs/phase_status/README.md](phase_status/README.md) and the per-phase files
for what is finished, started, and left to do.

## Project Shape

HockeyGame is a custom C++20 hockey game and engine project. The repo is split
into subsystem libraries under `libs/`, executable apps under `apps/`, runtime
data under `data/`, helper scripts under `scripts/`, and project documentation
under `docs/`.

The intended game is a real-time 4v4 hockey game:

- 2 teams
- 3 skaters and 1 goalie per team
- 8 player slots total
- 1 puck
- 2 goals
- dedicated authoritative server simulation
- client input, snapshots, prediction, interpolation, and reconciliation in the
  networking phase

## Target Graph

Current library targets:

```text
hockey_core
hockey_ecs        -> hockey_core
hockey_assets     -> hockey_core
hockey_renderer   -> hockey_core, hockey_ecs, hockey_assets
hockey_physics    -> hockey_core, hockey_ecs
hockey_gameplay   -> hockey_core, hockey_ecs, hockey_physics
hockey_editor     -> hockey_core, hockey_ecs, hockey_renderer,
                     hockey_assets, hockey_physics, hockey_gameplay
```

Current executable targets:

```text
HockeyGameClient      -> core, ecs, renderer, assets, physics, gameplay
HockeyMapEditor       -> core, ecs, renderer, assets, editor, physics, gameplay
HockeyDedicatedServer -> core, ecs, physics, gameplay
HockeyAssetTool       -> core, assets
HockeyShaderTool      -> core, renderer
```

Planned target:

```text
hockey_networking -> hockey_core, hockey_ecs
```

The dedicated server must stay headless. The root `CMakeLists.txt` contains
architecture guards that fail configuration if the server links renderer, editor,
Vulkan, ImGui, or other UI/rendering dependencies.

## Top-Level Layout

```text
.
|-- apps/              Executables, tools, and test runners
|-- data/              Runtime config, raw assets, cooked assets, shaders
|-- docs/              Project documentation, phase status, AI workflow notes
|-- libs/              Engine libraries
|-- scripts/           Windows and Linux setup/build/run/test scripts
|-- CMakeLists.txt     Top-level target wiring and architecture guards
|-- CMakePresets.json  Configure/build presets
|-- vcpkg.json         Manifest-mode third-party dependencies
|-- AGENTS.md          Agent rules and architecture constraints
|-- README.md          Short project entry point
|-- LICENSE            Project license
|-- .cursor/           Cursor project rules and active phase context
|-- .clang-format      Formatting rules
|-- .clang-tidy        Static analysis rules
|-- .editorconfig      Editor defaults
`-- project-words.txt  Project dictionary for spell checking
```

Common generated/local-output directories:

```text
build/            CMake/Ninja build outputs
out/              Ignored AI diagnostics and generated helper output
vcpkg_installed/  vcpkg installed packages
.vcpkg/           Optional repo-local vcpkg checkout/state
_save/            User/runtime saves and built-in screenshot output
data/logs/        Runtime logs
data/temp/        Temporary runtime/editor/test files
```

`_save/` is user-owned local runtime output. AI diagnostics should go under
`out/`, usually `out/ai_diagnostics/`.

## Layout Conventions

Libraries generally follow this structure:

```text
libs/<library>/
|-- CMakeLists.txt
|-- include/Hockey/<Subsystem>/  Public headers
`-- src/                         Private implementation
```

Apps generally follow this structure:

```text
apps/<app>/
|-- CMakeLists.txt
`-- src/                         Main, app class, commands, or tests
```

Rules of thumb:

- Public engine headers live under `include/Hockey/...`.
- Private implementation lives under each target's `src/` directory.
- Reusable behavior belongs in `libs/`, not in executable apps.
- Test runners are regular apps under `apps/*_tests`.
- Runtime path resolution is rooted with the `--root` command-line flag.
- Raw asset metadata lives beside raw files as `.meta.yaml`.
- Cooked assets are generated under `data/cooked/`.

## Applications

```text
apps/
|-- game_client/        Playable windowed client (`HockeyGameClient`)
|-- map_editor/         Unity-style editor (`HockeyMapEditor`)
|-- dedicated_server/   Headless authoritative simulation app
|-- asset_tool/         Asset pipeline command-line tool
|-- shader_tool/        Shader compilation command-line tool
|-- core_tests/         Core runtime test executable
|-- ecs_tests/          ECS/scene/prefab test executable
|-- asset_tests/        Asset pipeline test executable
|-- renderer_tests/     Renderer/settings/shader/render-graph tests
|-- editor_tests/       Editor workflow/tool/preview tests
|-- physics_tests/      Physics world/component/query tests
`-- gameplay_tests/     Hockey gameplay simulation tests
```

App ownership:

- `game_client` composes runtime systems for the playable windowed game path.
- `map_editor` hosts the editor app, editor config, viewport capture flags, and
  startup scene handling.
- `dedicated_server` hosts the headless server lifecycle and bounded tick runs.
- `asset_tool` exposes asset discovery/import/cook/validate workflows.
- `shader_tool` compiles shader sources into SPIR-V outputs.
- test apps validate subsystem behavior without adding cross-library ownership.

## Engine Libraries

```text
libs/
|-- core/       Platform/runtime foundation
|-- ecs/        Entity, component, scene, prefab, and validation data
|-- assets/     CPU-side asset import, cook, database, and runtime loading
|-- renderer/   Vulkan renderer, render resources, frame path, screenshots
|-- physics/    Jolt-backed physics facade and ECS physics sync
|-- gameplay/   Hockey rules, input, match, puck, player, and snapshot systems
`-- editor/     Dear ImGui editor, panels, tools, gizmos, previews
```

Library boundaries:

- `core` owns platform/runtime basics: paths, logging, config, input,
  application loops, jobs, timing, and screenshot path generation.
- `ecs` owns entities, components, scenes, prefabs, metadata, serialization, and
  validation data. It should stay renderer/UI/platform agnostic.
- `assets` owns CPU-side content import, cooking, metadata, databases, and
  runtime asset loading. It does not upload GPU resources directly.
- `renderer` owns Vulkan, GPU resources, render settings, render targets, debug
  drawing, and screenshot readback.
- `physics` owns simulation through a Jolt-hidden public facade.
- `gameplay` owns hockey rules, match flow, player/puck systems, tuning, and
  snapshots.
- `editor` owns editor UI, panels, tools, gizmos, project browser, previews, and
  editor workflow code.

Useful subdirectories:

```text
libs/renderer/src/Vulkan/       Vulkan implementation details
libs/physics/src/Jolt/          Jolt implementation details
libs/gameplay/src/Match/        Match flow and scoring systems
libs/gameplay/src/Player/       Player movement and player state systems
libs/gameplay/src/Puck/         Puck and possession systems
libs/editor/src/Panels/         Editor panels
libs/editor/src/Tools/          Editor tools
libs/editor/src/Gizmos/         Editor viewport gizmos
libs/editor/src/Inspector/      Inspector field/component drawing
```

## Runtime Data

```text
data/
|-- config/       App config TOML files
|-- gameplay/     Default gameplay rules and tuning YAML
|-- raw/          Source assets and authoring files
|-- cooked/       Cooked asset database and cooked asset payloads
|-- editor/       User/editor settings and layout state
|-- shaders/      GLSL sources and compiled SPIR-V binaries
|-- logs/         Runtime log output
`-- temp/         Temporary files used by tests/editor/runtime flows
```

Raw asset layout:

```text
data/raw/
|-- scenes/       Authoring scenes and renderer/physics/gameplay test scenes
|-- prefabs/      Authored reusable entity hierarchies
|-- materials/    Material YAML assets and metadata
|-- models/       GLB models and extracted texture folders
`-- textures/     Standalone texture assets and metadata
```

Shader layout:

```text
data/shaders/
|-- src/          GLSL source files
`-- bin/          Compiled SPIR-V files consumed by the renderer
```

Common shader source groups:

- mesh/PBR: `mesh.vert`, `pbr.frag`, `common.glsl`
- shadow/depth: `depth_only.vert`, `shadow.vert`, `shadow.frag`
- post: `ssao.frag`, `ssao_blur.frag`, `bloom_*`, `tonemap.frag`, `fxaa.frag`
- debug: `debug_line.*`, `debug_shape.*`

## Documentation

```text
docs/
|-- project-structure.md             This repository structure guide
|-- ai-onboarding.md                 AI orientation checklist
|-- ai-debugging-playbook.md         Debug/build/test/screenshot workflow
|-- phase_status/                    Per-phase checkbox status
`-- phase-rules/                     Phase-specific Cursor rule sources
```

Phase status files:

```text
docs/phase_status/
|-- README.md
|-- phase-01-foundation-core.md
|-- phase-02-ecs-scene-prefab.md
|-- phase-03-vulkan-renderer.md
|-- phase-04-unity-style-editor.md
|-- phase-05-asset-pipeline.md
|-- phase-06-physics.md
|-- phase-07-hockey-gameplay.md
|-- phase-08-networking-lobbies.md
`-- phase-09-polish-animation-audio-ui.md
```

Before making status-sensitive changes, read `docs/phase_status/README.md` and
the phase file for the subsystem being touched.

## Scripts

Windows scripts:

```text
scripts/windows/
|-- setup.ps1
|-- configure_debug.ps1
|-- configure_release.ps1
|-- build_debug.ps1
|-- build_release.ps1
|-- build_shaders.ps1
|-- ai_smoke.ps1
|-- run_client.ps1
|-- run_editor.ps1
|-- run_server.ps1
|-- test.ps1
|-- Env.ps1
|-- Prerequisites.ps1
`-- README.md
```

Linux scripts:

```text
scripts/linux/
|-- setup.sh
|-- configure_debug.sh
|-- configure_release.sh
|-- build_debug.sh
|-- build_release.sh
|-- ai_smoke.sh
|-- run_client.sh
|-- run_editor.sh
|-- run_server.sh
`-- test.sh
```

Script roles:

- `setup` checks/bootstrap tools and dependencies.
- `configure_*` runs the matching CMake configure preset.
- `build_*` runs the matching CMake build preset.
- `test` runs the configured test suite for that platform.
- `run_client`, `run_editor`, and `run_server` launch built debug apps with
  `--root` pointing at the repo.
- `build_shaders.ps1` builds/runs the shader tool on Windows.
- `ai_smoke` runs bounded text/visual smoke workflows and stores diagnostics in
  `out/ai_diagnostics/`.

## Build And Run Reference

Windows:

```powershell
.\scripts\windows\setup.ps1
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

Linux:

```bash
./scripts/linux/setup.sh
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
```

Run apps:

```powershell
.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1
```

Linux wrappers live in `scripts/linux/`. Use direct executable commands when
passing custom Linux flags if a wrapper does not forward arguments.

Build outputs:

```text
build/windows-debug/apps/<app>/
build/windows-release/apps/<app>/
build/linux-debug/apps/<app>/
build/linux-release/apps/<app>/
```

## Common App Flags

Game client:

```text
--root <dir>
--config <file>
--log <file>
--fps-limit <n>
--max-frames <n>
--screenshot [prefix]
--screenshot-prefix <p>
--vk-validation
--physics-debug
--help, -h
```

Map editor:

```text
--root <dir>
--config <file>
--log <file>
--scene <file>
--max-frames <n>
--frames <n>
--capture-viewports
--capture-scene-view
--capture-game-view
--capture-prefix <p>
--capture-width <n>
--capture-height <n>
--screenshot
--screenshot-window
--screenshot-prefix <p>
--screenshot-frame <n>
--vk-validation
--help, -h
```

Dedicated server:

```text
--root <dir>
--config <file>
--log <file>
--max-ticks <n>
--help, -h
```

## Screenshot Mechanics

Built-in screenshot paths are generated through `Hockey::MakeScreenshotPath()`
and write PNG files under:

```text
_save/screenshots/
```

Filename format:

```text
<sanitized-prefix>_YYYYMMDD_HHMMSS.png
```

Prefix rules:

- Alphanumeric characters are lowercased.
- `-` and `_` are preserved.
- Other characters collapse to `_`.
- A trailing `_` is removed.
- An empty prefix becomes `screenshot`.
- Existing filenames get `_1`, `_2`, and so on.

Game client screenshots:

- Press F12 while running for a `game` screenshot.
- Use `--screenshot [prefix]` for first-frame capture.
- Use `--screenshot-prefix <prefix>` to set the automated prefix.

Map editor screenshots:

- Use `--screenshot` for a full editor-window screenshot.
- Use `--screenshot-frame <n>` to delay full-window capture.
- Use `--capture-viewports` to capture both Scene and Game viewports.
- Use `--capture-scene-view` or `--capture-game-view` for one viewport.
- Use `--capture-width <n>` and `--capture-height <n>` for automated viewport
  capture resolution.
- Manual viewport captures use the `Capture` overlay button or F12 while the
  Scene/Game viewport is hovered.

Screenshot implementation notes:

- Requests are queued and captured during rendering.
- Full-window screenshots copy from the swapchain image.
- Editor viewport screenshots copy from renderer-owned offscreen render targets.
- PNG writing supports 8-bit RGBA/BGRA render targets.
- Bounded screenshot runs should allow enough frames for capture/readback.

## Architecture Guardrails

Keep these boundaries intact:

- `core` must not depend on ECS, renderer, physics, gameplay, editor, Vulkan, or
  ImGui.
- `ecs` must not depend on renderer, physics, gameplay, editor UI, SDL windowing,
  Vulkan, or ImGui.
- `assets` must remain CPU-side and independent from renderer/editor/ECS/physics
  gameplay ownership.
- `renderer` must not know hockey gameplay rules.
- `physics` must not link renderer, editor, gameplay, Vulkan, or ImGui.
- `gameplay` must not directly use SDL3, Vulkan, ImGui, or editor UI.
- `editor` may inspect and preview gameplay/physics, but it should not own core
  simulation systems.
- `dedicated_server` must stay headless.
