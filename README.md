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
  - HDR offscreen pipeline: PBR forward, cascaded directional shadows, spot/point local-light shadow atlas (tiled depth + PCF), transparent pass
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

These are carry-over items within Phases 1–6. They do not block starting Phase 7, but should be tracked. Two gap-closing passes resolved the small/medium items in each phase (the `*(closed)*` bullets); each remaining open bullet is annotated with **_depends on:_** the concrete blocker that keeps it open. The blockers fall into a few recurring categories:

- **GPU verification** — code is writable, but correctness can only be confirmed on a real GPU/display (this CI/headless environment has neither), so shipping it unverified is riskier than tracking the gap.
- **Phase 7/8/9** — the item is a future-phase system; per phase discipline it is intentionally not implemented early.
- **New dependency / large subsystem** — requires a new third-party library or a multi-feature subsystem, not a gap-closer.
- **In-scope now** — genuinely doable within Phases 1–6 today; called out explicitly.

#### Phase 1 — Core

- *(closed)* `SignalHandler::Install()` now wired in `WindowedApplication` as well as `HeadlessApplication`; the windowed client/editor honor Ctrl+C/SIGTERM
- *(closed)* `TextInput` events are emitted from the window layer; `Window::StartTextInput()/StopTextInput()` gate SDL text input
- *(closed)* `glm` removed from `hockey_core` (now linked directly by `hockey_ecs`, which uses it in public headers)
- *(closed)* Added direct tests for `Platform`, `Timer`, `ThreadPool`, and `SignalHandler`
- *(closed)* `--config` / `--log` / `--help` command-line flags wired in the client, editor and server (`Paths::Resolve` anchors relative overrides at the project root)
- *(closed)* `app.sleep_when_idle` honored by `HeadlessApplication`; `input.gamepad_enabled` applied at startup (closes pads opened before config load)
- *(closed)* `CrashHandler::Shutdown()` called on app teardown; added `Time`, `Paths`, `JobSystem` and `CommandLine` tests
- **Untested:** window/SDL lifecycle, keyboard/mouse/gamepad input, crash handler, logging output, windowed app loop (require a display/GPU)

#### Phase 2 — ECS

- *(closed)* `ParentComponent` / `ChildrenComponent` registered in component metadata (read-only, non-addable/removable)
- *(closed)* Prefab overrides persist to scene YAML (`PrefabOverrideSet` owned by `Scene`, round-trip serialized)
- *(closed)* `SceneMode` is serialized in scene files (with safe fallback to `Edit`)
- *(closed)* Added active-in-hierarchy propagation, `Scene::Clear`, marker round-trip, and prefab-override persistence tests

#### Phase 3 — Renderer

- *(closed)* Spot-light cone falloff applied in the PBR shader
- *(closed)* `MeshRendererComponent::castsShadows` honored by the shadow pass
- *(closed)* Spot/point local-light shadows: tiled depth atlas (4×4 grid), per-light view-proj matrices, PCF sampling in `pbr.frag` (`spot_shadow_test` / `point_shadow_test` scenes)
- *(closed)* `cpuFrameMs` populated from a CPU frame timer
- Frame graph is hard-coded in `Renderer.cpp`; `RenderGraph` exists but is not the production path — _depends on:_ large renderer refactor to route all passes through `RenderGraph` + GPU verification
- Anti-aliasing: only FXAA is implemented; TAA and MSAA are settings/preset values only — _depends on:_ motion-vector + history-buffer plumbing (TAA) and render-pass/pipeline multisample changes (MSAA); GPU verification
- `MeshRendererComponent::receivesShadows` and `contactShadows` still unused — _depends on:_ shadow-sampling push-constant/descriptor layout change; GPU verification
- `ReflectionProbeComponent` and `DecalComponent` serialize but renderer ignores them — _depends on:_ IBL probe capture + decal projection renderer systems; GPU verification
- Most `RendererSettings` fields are persisted only; live anisotropy re-apply deferred — _depends on:_ runtime sampler/descriptor recreation; GPU verification
- Hockey-specific visuals (ice reflections, skate spray, puck trail) are material/settings names only — no rendering systems — _depends on:_ Phase 9 (visual polish) + gameplay state to drive them
- `gpuFrameMs` not populated; no Tracy profiling integration — _depends on:_ GPU timestamp queries (GPU verification) + Tracy integration
- **Untested:** minimize behavior, VMA directly, TAA/MSAA paths, pixel-correct rendering, spot/point shadow atlas correctness — _depends on:_ a GPU/display in the test environment

#### Phase 4 — Editor

- *(closed)* Dedicated **Preferences** panel edits the persisted `EditorSettings` (autosave, validation, grid/snap, camera, asset pipeline) and saves on change; opened from Edit → Preferences
- *(closed)* **Component** menu wires Add/Remove against the primary selection from the component registry
- *(closed)* Edit → **Select All** (and Ctrl+A / Esc deselect) implemented over `Selection::SelectAll`
- *(closed)* Viewport prefab drop places the instance at the cursor's ground-plane (y=0) intersection
- *(closed)* `PropertiesPanel` shows live scene properties (name, mode, entity/system counts, file path)
- *(closed)* Startup scene precedence: explicit `--scene` > restore-last-scene (`restoreLastScene`) > `scene.startup_scene` config
- *(closed)* "Create Prefab" is now an undoable `EditorCommand` that stamps the source entity as a prefab instance; Ctrl+P toggles Play mode; hierarchy context menu adds "Focus In Viewport"
- Toolbar Play/Simulate only flip `SceneMode` and call lifecycle hooks — no systems or physics attached; Pause/Step disabled — _depends on:_ Phase 7 gameplay/sim runtime wired into Play mode (Pause/Step is meaningless until something simulates)
- Physics simulation lives in the separate Physics panel preview, not in Play/Simulate modes — _depends on:_ Phase 7 (attaching the `PhysicsWorld` to Play mode)
- *(closed)* Prefab override **apply/revert** implemented: `PrefabSerializer::ComputeOverrides` diffs an instance against its prefab (populating `PrefabOverrideSet`), `ApplyInstanceToPrefab` merges instance edits back into the prefab file (preserving its asset id), and `RevertInstanceToPrefab` restores authored values. The Prefab panel shows the override count and wires Apply (direct) / Revert (undoable `RevertPrefabOverrides` command)
- **Untested:** all panel UI, viewport/gizmo/picking, play/simulate modes, physics preview panel, file dialogs, game viewport — _depends on:_ a display/GPU in the test environment

#### Phase 5 — Assets

- *(closed)* Tangents are generated (Lengyel's method) when a glTF primitive omits them
- *(closed)* `TextureImportSettings.maxSize` clamps texture dimensions (box-filtered downsample) at cook time
- *(closed)* Runtime `Load<SceneAsset>` / `Load<PrefabAsset>` return lightweight descriptors (id, name, source path, dependency ids) with type-checked caching
- *(closed)* Hot-reload emits a `Reloaded` event and evicts the stale runtime instance from the registry on recook
- *(closed)* `AssetDatabase::DetectCycle` flags circular dependency graphs; `ValidateReferences` reports them as an error
- *(closed)* Hot-reload move detection: a renamed/moved raw file emits a `Moved` event and keeps its asset id (and dependents) instead of being deleted + re-imported
- *(closed)* Editor host honors `assets.raw_path` / `assets.cooked_path` and the `assets.auto_discover/auto_import/auto_cook_dirty` config keys
- *(closed)* Project browser "Delete Metadata" action (`AssetManager::DeleteMetadata`) removes the sidecar + database record while leaving raw/cooked files intact
- No KTX/Basis compression path; `.ktx/.ktx2` classified but not truly supported — _depends on:_ a new third-party lib (libktx/basisu) + cooked-format plumbing
- `TextureImportSettings.compress` unused; no cubemap/environment map pipeline — _depends on:_ a GPU block-compression encoder + renderer cubemap/IBL pipeline; GPU verification
- `Audio` and `Animation` asset types are enum placeholders only — _depends on:_ Phase 9 (audio) / the animation system
- No LOD pipeline; no shader variant system; asset IDs are stable uint64, not `Hockey::UUID` — _depends on:_ large new subsystems (LOD/variants); the uint64 id is a deliberate design choice, not a defect
- *(closed)* Material editor exposes the five PBR texture slots (base color, normal, metallic/roughness, occlusion, emissive) with `AssetDragPayload` drop targets — dropping a texture asset sets the slot to its raw path; a Clear button empties it
- **Untested:** KTX/HDR cook, asset tool CLI, editor browser integration, cross-platform E2E workflow — _depends on:_ a display/GPU + a packaged cross-platform CI harness

#### Phase 6 — Physics

- *(closed)* Optional `PhysicsMaterialComponent` overrides the rigid body's material (serialized + metadata)
- *(closed)* Sphere/box `ShapeCast` query added (Jolt narrow-phase)
- *(closed)* `TriggerComponent` detect flags enforced by `PhysicsWorld::DrainTriggerEvents(Scene&)`
- *(closed)* `deterministicMode` and `integrationSubsteps` applied to the Jolt world (substep loop + deterministic settings)
- *(closed)* `HockeyPlayerTool` spawns player/goalie capsule bodies (RigidBody + CapsuleCollider + layer/material/markers)
- *(closed)* Editor `PhysicsGizmo` draws a placeholder box for mesh colliders
- *(closed)* Added `RaycastAll`, friction/bounce, substep, trigger detect-flag, and determinism tests
- *(closed)* Capsule `ShapeCast` shape (player-shaped sweeps) added alongside sphere/box
- *(closed)* Material combine modes (`CombineFriction` / `CombineRestitution`) applied at contacts via the Jolt contact listener; built-ins keep the historical max-restitution behaviour
- *(closed)* `Sensor` layer now collides with players/goalies/puck/stick (and never static geometry) so it works as a non-physical detection zone
- *(closed)* Validation warns when a player/goalie physics body omits a capsule collider or uses invalid capsule dimensions
- `CharacterControllerComponent` serializes but has no Jolt controller or simulation — _depends on:_ Phase 7 (Jolt `CharacterVirtual` integration + movement logic)
- *(closed)* `MeshColliderComponent` builds runtime shapes via an injected `PhysicsMeshRegistry` provider (convex → `ConvexHullShape`, concave → `MeshShape`); the editor installs a provider backed by its `AssetManager`, keeping `physics` independent of `assets`
- `SceneMode::ClientPrediction` unused; client runs `Play` with no prediction/reconciliation — _depends on:_ Phase 8 networking

#### Cross-cutting

- No `libs/gameplay` or `libs/networking` libraries yet — _depends on:_ Phases 7/8 (correct for the current phase)
- CMake architectural guards enforce headless server and dependency boundaries
- Automated tests are strong on library logic (`Phase2GapTests`, `Phase5GapTests`, `Phase6GapTests`, plus new core coverage for `Platform`, `Time`, `ThreadPool`); weak on SDL/GPU/UI integration and cross-platform smoke — _depends on:_ a display/GPU + a cross-platform CI harness

## License

[MIT License](LICENSE) — Copyright (c) 2026 Peter Farber.
