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

After every repo-tracked change, check whether `docs/phase-status/` needs to be
updated. If the change completes, starts, removes, invalidates, or changes the
verification state of a phase item, update the matching
`docs/phase-status/phase-*.md` file in the same change. If no phase status
changed, explicitly say "No phase status change needed" in the final response.

## Agent tooling files

Keep `AGENTS.md`. It is the authoritative instruction file for humans and AI
agents working in this repo.

`agents.toml` is the dotagents machine-readable manifest for Codex-oriented
tool and skill configuration. It does not replace `AGENTS.md`.

Supporting AI context belongs in `docs/ai/`, `docs/ai-rules/`,
`docs/phase-rules/`, and `docs/phase-plans/`. Treat those files as
supporting guidance unless they are promoted into this file.

Do not add repo-managed `.agents` skills just to mirror these instructions.
Only add a skill when the project has a repeatable workflow with concrete,
versionable instructions that should travel through dotagents.

## AI working loop

At the start of a coding task, read the relevant current source/CMake/tests and
the matching `docs/phase-status/` file before editing. Use `docs/ai/onboarding.md`
for the full orientation checklist when the task spans multiple subsystems.

Work directly on the current branch. Do not create git worktrees, do not create
new branches, and do not switch branches unless the user explicitly asks for
that git operation. If the current branch looks wrong, stop and ask before
changing it.

Prefer the root `justfile` command surface:

- `just tools-check` for local tool inventory.
- `just serena-health` for semantic tooling health.
- `just verify` as the normal completion contract for repo changes.

Before finalizing, report the focused verification that ran and whether phase
status changed. If verification could not run, report the exact blocker.

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
- `libs/gameplay`: hockey simulation/rules only.
- `libs/networking`: transport/protocol/replication/lobbies only (future Phase 8).
- `libs/editor`: editor panels/tools only.

Apps:

- `apps/game_client`: playable game.
- `apps/map_editor`: Unity-style map editor.
- `apps/dedicated_server`: headless authoritative server.
- `apps/core_tests`: tests for core.
- `apps/ecs_tests`: tests for ECS/scene/prefab behavior.
- `apps/asset_tests`: tests for the asset pipeline.
- `apps/renderer_tests`: tests for renderer settings, shaders, render graph, and smoke behavior.
- `apps/editor_tests`: tests for editor logic, tools, and previews.
- `apps/physics_tests`: tests for physics behavior.
- `apps/gameplay_tests`: tests for hockey gameplay simulation.
- `apps/asset_tool`: asset discovery/import/cook/validation CLI.
- `apps/shader_tool`: shader compilation CLI.

## Dependency direction

The dependency graph must remain clean:

```text
core
ecs        -> core
assets     -> core
renderer   -> core, ecs       (privately consumes assets)
physics    -> core, ecs
gameplay   -> core, ecs, physics
editor     -> core, ecs, renderer, assets, physics, gameplay

game_client       -> core, ecs, renderer, assets, physics, gameplay
map_editor        -> core, ecs, renderer, assets, editor, physics, gameplay
dedicated_server  -> core, ecs, physics, gameplay
```

Future Phase 8 networking dependency direction:

```text
networking -> core, ecs
game_client       -> networking
dedicated_server  -> networking
```

## Hard rules

- `core` must not depend on ECS, renderer, physics, networking, gameplay, editor, Vulkan, ImGui, or GameNetworkingSockets.
- `ecs` must not depend on renderer, physics, networking, gameplay, editor UI, SDL windowing, Vulkan, or ImGui.
- `renderer` must not know hockey gameplay rules.
- `networking` must not depend on renderer.
- `gameplay` must not directly use SDL3, Vulkan, or ImGui.
- `gameplay` must not depend on networking until Phase 8 explicitly introduces that integration.
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

<!-- BEGIN AI BOOTSTRAP TOOLING -->
## Bootstrap-installed local AI tooling

The external Windows AI bootstrap may install developer-local tools and skills
for this checkout. These tools accelerate work but do not replace repository
source, CMake configuration, tests, phase status, logs, captures, or screenshots
as the source of truth.

For cross-subsystem work, read `docs/ai/onboarding.md`. Follow
`docs/ai/workflow.md` and `docs/ai/debugging.md` when applicable.
Read the matching `docs/phase-status/`, `docs/phase-rules/`, and
`docs/phase-plans/` files before changing a phase-owned subsystem.

### Local skills

Use an installed local skill when it matches the task:

- `vulkan-renderer`: Vulkan lifetime, synchronization, descriptors, pipelines,
  swapchain, GPU resources, and validation.
- `shader-programming`: shader interfaces, GLSL/HLSL/SPIR-V, PBR, lighting,
  shadows, post-processing, and material GPU data.
- `renderdoc-debugging`: evidence-driven frame and resource inspection.
- `tracy-profiling`: CPU/GPU performance work backed by measurements.
- `blender-asset-pipeline`: Blender MCP and game-ready asset preparation.
- `hockey-gameplay`: hockey rules, scoring, possession, player actions, and game
  state.
- `hockey-physics`: puck/player movement, collisions, fixed timestep, and
  deterministic simulation concerns.
- `hockey-ai-behavior`: CPU skaters, goalies, positioning, steering, tactics,
  and difficulty tuning.
- `hockey-networking`: authoritative server, input commands, snapshots,
  prediction, reconciliation, serialization, and finite network tests.
- `hockey-arena-editor`: rink data, goals, boards, spawns, validation,
  serialization, and editor/runtime agreement.
- `hockey-asset-pipeline`: hockey-specific scale, orientation, naming,
  collision proxies, and import/export expectations.

Local skills provide repeatable workflows. They do not override this file,
project architecture, phase discipline, or explicit user instructions.

### Serena

Use Serena for semantic C++ symbol lookup, references, diagnostics, and precise
symbol-level edits. Ensure it uses a project compilation database rather than a
`compile_commands.json` generated only for a dependency.

### Beads

Use Beads for multi-session features, bugs, blockers, dependencies, and
follow-up work discovered during implementation. Do not create a Beads task for
trivial one-line edits. Beads state under `.beads/` is local unless explicitly
exported or shared.

### Graphify

Graphify is configured for on-demand architecture and impact analysis. Use it
manually before broad refactors, dependency tracing, or unfamiliar subsystem
work, then verify its conclusions against source and semantic references. If
generated community labels are generic, use `just graphify-name-communities`.

### Local-only state

Do not commit generated local AI/tool state:

- `.ai/`
- `.agents/`
- `.codex/`
- `.cursor/`
- `.beads/`
- `.serena/`
- `.graphifyignore`
- `graphify-out/`
- `.mcp.json`
- `agents.lock`

The tracked `AGENTS.md`, project documentation, `.gitignore`, and `justfile`
remain the portable repository guidance and command surface.
<!-- END AI BOOTSTRAP TOOLING -->
