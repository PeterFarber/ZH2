# HockeyGame

Cross-platform C++20 hockey game and engine targeting a real-time 4v4 experience with an authoritative dedicated server, Vulkan renderer, Unity-style map editor, and modular library architecture.

**Current status:** Phases 1–6 are substantially implemented (foundation through physics). Remaining work within those phases is documented below under [Known gaps](#known-gaps-within-completed-phases). Gameplay, networking, and polish (Phases 7–9) have not started.

## Game design

- 2 teams, 3 skaters + 1 goalie each (8 players total)
- 1 puck, 2 goals
- Dedicated authoritative server simulates the match
- Clients send input and receive snapshots
- Client prediction, interpolation, and reconciliation planned for Phase 8

## Architecture

Libraries follow a strict dependency graph:

```text
core
ecs -> core
assets -> core
renderer -> core, ecs, assets
physics -> core, ecs
editor -> core, ecs, renderer, assets, physics
gameplay -> core, ecs, physics, networking   (Phase 7 — not started)
networking -> core, ecs                      (Phase 8 — not started)

game_client -> core, ecs, renderer, assets, physics
map_editor -> core, ecs, renderer, assets, editor, physics
dedicated_server -> core, ecs, physics       (headless — no renderer/editor)
```

| Library | CMake target | Purpose |
|---------|--------------|---------|
| Core | `hockey_core` | Platform, logging, config, paths, input, windowing, jobs, timing |
| ECS | `hockey_ecs` | Entities, components, scenes, prefabs, serialization |
| Assets | `hockey_assets` | Import, cook, load, hot-reload asset pipeline |
| Renderer | `hockey_renderer` | Vulkan PBR renderer, post-processing, debug draw |
| Editor | `hockey_editor` | Unity-style map editor (ImGui) |
| Physics | `hockey_physics` | Jolt Physics integration, collision, queries |

## Applications

| App | Description |
|-----|-------------|
| `HockeyGameClient` | Windowed client — renderer, Play-mode scene load, fixed-step physics, optional physics debug draw |
| `HockeyMapEditor` | Full editor UI, asset import/cook/watch, offscreen viewport, hockey authoring tools, opt-in physics preview panel |
| `HockeyDedicatedServer` | Headless fixed-tick server with Server-mode scene load and authoritative physics |
| `HockeyAssetTool` | CLI for asset discovery, import, cook, and validation |
| `HockeyCoreTests` | Core foundation unit tests |
| `HockeyECSTests` | ECS, scene, prefab, and serialization tests |
| `HockeyRendererTests` | Renderer settings/shader compile + optional GPU smoke tests |
| `HockeyEditorTests` | Editor logic tests (no GPU window) |
| `HockeyAssetTests` | Asset pipeline tests |
| `HockeyPhysicsTests` | Physics world, colliders, queries, events, and headless server-path tests |

## Third-party dependencies

Managed via [vcpkg](https://vcpkg.io/) manifest mode (`vcpkg.json`).

| Package | Used for |
|---------|----------|
| **SDL3** (+ Vulkan feature) | Window, events, input, gamepad |
| **spdlog** / **fmt** | Logging and formatting |
| **glm** | Math (renderer/ECS; linked in core but unused there) |
| **tomlplusplus** | Config files |
| **entt** | ECS registry |
| **yaml-cpp** | Scene/prefab/asset YAML serialization |
| **volk** | Vulkan loader |
| **vulkan-headers** | Vulkan API |
| **vulkan-memory-allocator** | GPU memory allocation |
| **shaderc** | GLSL → SPIR-V shader compilation |
| **spirv-cross** / **spirv-tools** | Declared in manifest; not linked or used yet |
| **stb** | Image loading (assets); declared for renderer but unused there |
| **fastgltf** | glTF/GLB model import |
| **meshoptimizer** | Mesh cooking/optimization |
| **joltphysics** | Rigid-body physics simulation |
| **imgui** (+ SDL3 binding, docking) | Editor UI |
| **imguizmo** | Transform gizmos in editor |
| **nativefiledialog-extended** | Native open/save dialogs |

Build tools: **CMake 3.25+**, **Ninja**, **C++20** compiler (GCC, Clang, or MSVC).

## Build

Set `VCPKG_ROOT` to your vcpkg installation, then configure and build with CMake presets.

**Linux (first time):**

```bash
./scripts/linux/setup.sh          # optional: system deps + vcpkg bootstrap
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
```

**Linux (run apps):**

```bash
./scripts/linux/run_editor.sh
./scripts/linux/run_client.sh
./scripts/linux/run_server.sh
```

**Windows:**

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
```

Presets: `linux-debug`, `linux-release`, `windows-debug`, `windows-release`.

## Project layout

```text
apps/           Executables (client, editor, server, tools, tests)
libs/           Engine libraries (core, ecs, assets, renderer, editor, physics)
data/           Config, raw/cooked assets, shaders, editor themes
scripts/        Linux and Windows build/run helpers
docs/           Phase rule reference files
.cursor/        Cursor AI rules and active phase config
```

## Development phases

Work proceeds in ordered phases. Each phase should fully complete its subsystem before moving on. See `AGENTS.md` and `.cursor/rules/` for agent/IDE guidance.

### Implemented (Phases 1–6)

- [x] **Phase 1 — Foundation / Core**
  - `Status`/`Result`, assertions, logging (spdlog + file sinks), TOML config, paths, filesystem
  - Platform abstraction (Windows/Linux): executable path, sleep, thread count, debugger detection
  - Signal handling (SIGINT/SIGTERM) and crash/terminate hook — wired in headless apps
  - CLI parsing, UUID, time/timer, fixed timestep, typed event queue
  - SDL3 window, keyboard/mouse/gamepad input, windowed and headless application loops
  - Thread pool and job system (`ParallelFor`)
  - CMake presets, vcpkg manifest, cross-platform build/run scripts

- [x] **Phase 2 — ECS, Scene, Prefab**
  - EnTT entity/component API: create, destroy, duplicate, find by UUID/name
  - Hierarchy, local/world transforms, active-in-hierarchy propagation
  - Scene modes (`Edit`, `Play`, `Server`, `ClientPrediction`), lifecycle hooks, system list
  - YAML scene/prefab serialization; scene validation (UUID, hierarchy, hockey warnings)
  - Prefab save/instantiate with UUID remap; in-memory prefab override apply
  - Component metadata registry for editor inspector
  - All 16 Phase 2 hockey marker components + Phase 3 render components

- [x] **Phase 3 — Vulkan Renderer**
  - Full Vulkan stack: instance, validation, surface, device, swapchain, VMA, Volk
  - HDR offscreen pipeline: PBR forward, cascaded directional shadows, transparent pass
  - Post-processing: SSAO + blur, bloom, tone mapping (Linear/Reinhard/ACES), FXAA
  - Procedural built-in meshes and hockey materials; asset-backed mesh/material resolve
  - ECS camera/mesh/light/environment components consumed at render time
  - Debug draw (lines, boxes, spheres, capsules), renderer settings + TOML I/O
  - Offscreen render targets for editor viewport; GPU smoke tests

- [x] **Phase 4 — Unity-Style Editor**
  - ImGui dockspace, main menu, toolbar, panel manager
  - Panels: Hierarchy, Inspector, Scene/Game viewports, Project, Console, Stats, Scene Validation, Prefab, Physics
  - Scene new/open/save/save-as, autosave, recent scenes, dirty prompts, validation on save
  - Undo/redo, selection, clipboard, metadata-driven inspector (bool/int/float/string/vec/enum/UUID/path/asset ref)
  - Editor camera, grid, ImGuizmo gizmos, AABB entity picking, focus selected
  - Hockey tools: rink (+ play area + board colliders), spawns, goals, puck, faceoffs, camera rig
  - Asset drag-drop, hot-reload polling, prefab create/instantiate
  - Separate Physics panel with opt-in preview sim (play/pause/step/reset) and viewport collider overlay

- [x] **Phase 5 — Asset Pipeline**
  - Asset IDs, handles, metadata sidecars, YAML asset database, dependency graph
  - Importers: glTF/GLB (fastgltf), textures (stb), materials, shaders, scenes, prefabs
  - Cookers: mesh optimization, SPIR-V shader cook, cooked binary formats
  - Runtime loaders for texture/mesh/material/model/shader; reference validation
  - File-watcher hot-reload with dirty propagation and editor auto-recook hooks
  - `HockeyAssetTool` CLI (discover, import, cook, validate, list)

- [x] **Phase 6 — Physics**
  - Jolt Physics world with fixed-step simulation and config-driven settings
  - Rigid bodies (static/kinematic/dynamic), box/sphere/capsule/cylinder colliders, triggers
  - Collision layers and filter matrix (player, goalie, puck, rink, goal, trigger, etc.)
  - Material registry with hockey presets (ice, puck rubber, boards, glass, etc.) via `RigidBodyComponent.materialName`
  - Raycast, raycast-all, overlap sphere/box; contact and trigger event drain APIs
  - Scene ↔ physics sync; ECS physics components with YAML serialize + inspector metadata
  - Debug draw collection; editor physics preview + viewport gizmo overlay
  - Client (`Play` mode) and dedicated server (`Server` mode) both step physics on fixed tick

### Remaining (Phases 7–9)

- [ ] **Phase 7 — Hockey Gameplay**
  - `hockey_gameplay` library
  - Player/skater/goalie movement and controls
  - Puck handling, passing, shooting, faceoffs
  - Rules, scoring, penalties, game state machine
  - Rink-bound gameplay simulation on top of physics

- [ ] **Phase 8 — Networking & Dedicated Server**
  - `hockey_networking` library (GameNetworkingSockets)
  - Protocol, snapshots, input replication
  - Lobbies, matchmaking hooks
  - Client prediction, interpolation, reconciliation
  - Full authoritative server ↔ client loop

- [ ] **Phase 9 — Polish (Animation, Audio, UI)**
  - Animation system and skeletal playback
  - Audio engine and spatial sound
  - In-game UI/HUD, menus, settings
  - Visual polish (ice effects, trails, crowd, etc.)

### Known gaps within completed phases

These are carry-over items within Phases 1–6. They do not block starting Phase 7, but should be tracked.

#### Phase 1 — Core

- `SignalHandler::Install()` only wired in `HeadlessApplication`; windowed client/editor do not hook Ctrl+C/SIGTERM
- `TextInput` event type declared but never emitted from the window layer
- `glm` linked in `hockey_core` but unused there
- **Untested:** window/SDL lifecycle, keyboard/mouse/gamepad input, signal handling, crash handler, logging output, `WindowedApplication`/`HeadlessApplication` loops, `Platform`, `Timer`, `ThreadPool` (direct)

#### Phase 2 — ECS

- `ParentComponent` / `ChildrenComponent` not registered in component metadata (inspector can't edit them directly)
- Prefab overrides are apply-only in memory — no save/load persistence in scene/prefab YAML
- `SceneMode` is runtime-only; not serialized in scene files
- **Untested:** active-in-hierarchy propagation, per-component serialization for all 16 components, prefab override round-trip, `Scene::Clear`, `SceneMode` transitions

#### Phase 3 — Renderer

- Frame graph is hard-coded in `Renderer.cpp`; `RenderGraph` exists but is not the production path
- Anti-aliasing: only FXAA is implemented; TAA and MSAA are settings/preset values only
- Spot-light cone falloff not applied in PBR shader; point/spot shadow maps not implemented
- `MeshRendererComponent::castsShadows` / `receivesShadows` unused; `contactShadows` setting unused
- `ReflectionProbeComponent` and `DecalComponent` serialize but renderer ignores them
- Most `RendererSettings` fields are persisted only (render scale, upscalers, texture/material quality, hockey visual quality knobs, motion blur, DOF, debug overlays, etc.)
- Hockey-specific visuals (ice reflections, skate spray, puck trail) are material/settings names only — no rendering systems
- `cpuFrameMs` / `gpuFrameMs` stats never populated; no Tracy profiling integration
- **Untested:** minimize behavior, VMA directly, TAA/MSAA paths, pixel-correct rendering, spot cones, point shadows

#### Phase 4 — Editor

- No dedicated Settings/Preferences panel; most editor prefs (snap values, camera speed, autosave) persist to TOML but aren't editable in UI (toolbar only exposes snap/grid toggles)
- Toolbar Play/Simulate only flip `SceneMode` and call lifecycle hooks — no systems or physics attached; Pause/Step disabled
- Physics simulation lives in the separate Physics panel preview, not in Play/Simulate modes
- Prefab override apply/revert UI disabled ("not implemented in the ECS yet")
- Viewport prefab drop spawns at origin only
- Component menu and Edit → Select All are stubbed
- `PropertiesPanel` is a placeholder and not in the default dock layout
- **Untested:** all panel UI, viewport/gizmo/picking, play/simulate modes, physics preview panel, file dialogs, game viewport

#### Phase 5 — Assets

- No KTX/Basis compression path; `.ktx/.ktx2` classified but not truly supported
- `TextureImportSettings.compress` and `maxSize` unused; no cubemap/environment map pipeline
- `Audio` and `Animation` asset types are enum placeholders only
- No tangent generation when glTF omits tangents; no LOD pipeline
- Scene/prefab import and cook work, but no runtime `Load<SceneAsset>` / `Load<PrefabAsset>`
- No shader variant system; asset IDs are stable uint64, not `Hockey::UUID`
- Material editor is scalar PBR fields only — no texture-slot drag-drop in UI
- Hot-reload `Reloaded` event type exists but is not emitted on recook
- **Untested:** scene/prefab import/cook, KTX/HDR cook, tangent generation, asset tool CLI, editor browser integration, cross-platform E2E workflow

#### Phase 6 — Physics

- No `PhysicsMaterialComponent` — materials selected via `RigidBodyComponent.materialName` + registry
- No shape casts
- `CharacterControllerComponent` serializes but has no Jolt controller or simulation
- `MeshColliderComponent` validates/serializes but cannot build shapes at runtime (no mesh asset bridge)
- `TriggerComponent` detect flags (`detectPlayers`, `detectGoalies`, `detectPuck`) not enforced — layer matrix only
- `deterministicMode` and `integrationSubsteps` config fields stored but not applied to Jolt
- `SceneMode::ClientPrediction` unused; client runs `Play` with no prediction/reconciliation
- No player/goalie capsule spawn tooling or default physics bodies (materials/layers exist)
- Editor `PhysicsGizmo` skips mesh collider visualization
- **Untested:** puck bounce/friction simulation sanity, `RaycastAll`, fixed-timestep accumulator multi-step, trigger detect flags at runtime, integrated rink+puck+goal sim

#### Cross-cutting

- No `libs/gameplay` or `libs/networking` libraries yet (correct for current phase)
- CMake architectural guards enforce headless server and dependency boundaries
- Automated tests are strong on library logic; weak on SDL/GPU/UI integration and cross-platform smoke

## License

[MIT License](LICENSE) — Copyright (c) 2026 Peter Farber.
