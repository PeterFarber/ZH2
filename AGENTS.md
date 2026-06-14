# HockeyGame Agent Instructions

This repository is a custom C++ hockey game/engine project.

## Project goal

Build a high-quality real-time 4v4 hockey game with:

- Vulkan renderer
- SDL3 platform/window/input layer
- Unity-style map editor
- Entity Component System
- Dedicated authoritative server
- GameNetworkingSockets networking
- Cross-platform Windows/Linux support

The game design:

- 2 teams
- 3 skaters + 1 goalie per team
- 8 players total
- 1 puck
- 2 goals
- Dedicated server authoritative simulation
- Clients send input and receive snapshots
- Real-time networking with prediction/interpolation/reconciliation

## Development model

Development is phase-based.

Each phase should fully complete its subsystem before moving on.

Do not implement systems from future phases unless explicitly requested.

No placeholders unless the user explicitly asks for temporary scaffolding.

## Phase order

1. Complete cross-platform foundation/core.
2. Complete ECS, GameObject model, scene system, prefabs, component metadata.
3. Complete Vulkan renderer subsystem.
4. Complete Unity-style editor.
5. Complete asset pipeline.
6. Complete physics.
7. Complete hockey gameplay simulation.
8. Complete networking, dedicated server, lobbies.
9. Complete animation/audio/UI/polish.

## Architecture

Respect these library boundaries:

- `libs/core`: platform/runtime basics only.
- `libs/ecs`: entity/component/scene/prefab data only.
- `libs/assets`: asset import/loading/cooking only.
- `libs/renderer`: Vulkan rendering only.
- `libs/physics`: physics integration only.
- `libs/networking`: transport/protocol/replication/lobbies only.
- `libs/gameplay`: hockey simulation/rules only.
- `libs/editor`: editor panels/tools only.

Apps:

- `apps/game_client`: playable game.
- `apps/map_editor`: Unity-style map editor.
- `apps/dedicated_server`: headless authoritative server.
- `apps/core_tests`: tests for core.
- Future test apps may be added per subsystem.

## Dependency direction

The dependency graph must remain clean:

```text
core
ecs        -> core
assets     -> core
renderer   -> core, ecs, assets
physics    -> core, ecs
networking -> core, ecs
gameplay   -> core, ecs, physics, networking
editor     -> core, ecs, renderer, assets, gameplay

game_client       -> core, ecs, renderer, assets, gameplay, networking
map_editor        -> core, ecs, renderer, assets, editor
dedicated_server  -> core, ecs, gameplay, networking, physics
```

## Hard rules

- `core` must not depend on ECS, renderer, physics, networking, gameplay, editor, Vulkan, ImGui, or GameNetworkingSockets.
- `ecs` must not depend on renderer, physics, networking, gameplay, editor UI, SDL windowing, Vulkan, or ImGui.
- `renderer` must not know hockey gameplay rules.
- `networking` must not depend on renderer.
- `gameplay` must not directly use SDL3, Vulkan, or ImGui.
- `editor` may inspect gameplay data but should not own core gameplay simulation.
- `dedicated_server` must stay headless and must not link renderer, Vulkan, ImGui, audio, or editor UI.

## Cross-platform rules

Support Windows and Linux.

Use:

- C++20
- CMake
- Ninja
- vcpkg manifest mode
- SDL3 for window/input/gamepad
- `std::filesystem` for paths
- `std::chrono::steady_clock` for timing
- Platform abstraction for OS-specific code
- CMake presets for Windows/Linux
- PowerShell scripts for Windows
- Bash scripts for Linux

Avoid:

- Hardcoded Windows paths
- Hardcoded `.exe` in runtime logic
- Backslash-only paths
- Direct Windows/Linux APIs outside platform abstraction files
- Assuming current working directory is project root
- Renderer dependencies in server code

## Coding expectations

- Use C++20 unless the user explicitly upgrades to C++23.
- Use RAII.
- Avoid raw owning pointers.
- Prefer `std::unique_ptr` for ownership.
- Prefer `std::shared_ptr` only when shared ownership is real.
- Prefer `Status` / `Result<T>` for recoverable engine failures.
- Use assertions only for programmer errors.
- Keep files small and focused.
- Update CMake when adding files.
- Add/update tests for core behavior.
- Keep Windows/Linux compatibility.
- Do not create monolithic systems.
- Do not silently change architecture.
