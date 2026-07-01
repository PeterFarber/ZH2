# Phase 9 - Polish, Animation, Audio, UI

Status: Started for runtime RmlUi UI foundation, first-pass audio system/event routing, editor-editable hockey audio cues, animation target/test scaffolding, first-pass animation graph/runtime evaluation, first-pass hockey animation controller integration, package scripting/profile UI, editor-owned runtime config packaging, and code-owned runtime config fallbacks. Renderer skinning, editor animation preview, final audio polish, full UI polish, embedded package runtime loading, and packaged app smoke coverage remain incomplete.

Source material:

- [x] Read `docs/phase-plans/phase-09-polish-animation-audio-ui.md`.
- [x] Read `docs/phase-rules/130-phase-9-polish-animation-audio-ui.mdc`.
- [x] Checked current source/CMake/test state.
- [x] Checked `docs/project-structure.md`.

## What This Phase Implements

- [x] `hockey_animation` target.
- [x] `hockey_audio` target.
- [x] `hockey_ui` target.
- [x] `HockeyUITests` covers runtime UI foundation behavior.
- [ ] Animation clips, skeletons, poses, blending, first-pass state machine/runtime evaluation, and first-pass hockey animation controller mapping exist; renderer skinning, editor preview, and runtime prefab wiring remain incomplete.
- [x] First-pass audio device, mixer, buses, clips, sources, and hockey audio event controller.
- [ ] UI framework, UI rendering, widgets, screens, HUD, overlays, and settings.
- [ ] Input rebinding, controller UX, accessibility, profiling, optimization, packaging, QA, and soak testing.

## Finished Prerequisites From Earlier Phases

- [x] Renderer has material/settings names for some future hockey visuals.
- [x] Renderer can draw the game scene and editor viewports.
- [x] Renderer screenshot support exists for visual verification.
- [x] Asset type placeholders exist for audio.
- [x] Asset type support exists for first-pass animation skeleton and clip assets.
- [x] Editor UI exists, but it is not the final in-game UI system.
- [x] Gameplay exposes match, player, puck, score, clock, goal, and event state for future HUD/audio/animation systems.
- [x] Networking is planned to provide lobby/team/role/network stats data before final UI polish is complete.

## Not Started - Target And Dependency Setup

- [x] Create `libs/animation/`.
- [x] Create `libs/audio/`.
- [x] Create `libs/ui/`.
- [x] Add CMake targets for animation.
- [x] Add CMake target for audio.
- [x] Add CMake target for UI.
- [x] Add app link updates for editor UI preview where appropriate.
- [x] Link the game client and map editor to `hockey_animation` for future runtime/editor animation integration.
- [x] Link the game client to `hockey_ui`.
- [ ] Add package targets/scripts.
- [ ] Add tests for animation/polish systems beyond the current `HockeyAnimationTests` target/settings/skeleton/clip/sampling/blending/component-serialization/graph/runtime contract coverage.
- [x] Add tests for audio systems.
- [x] Add tests for UI settings, client-flow assets, RmlUi interfaces/context/input mapping, and architecture guards.
- [x] Keep dedicated server free of renderer, editor UI, game UI rendering, and audio playback.

## Not Started - Animation

- [x] Add animation settings.
- [x] Add skeleton representation.
- [x] Add joint hierarchy support.
- [x] Add animation clip representation.
- [x] Add pose data.
- [x] Add clip sampling.
- [x] Add pose blending.
- [x] Add animation components.
- [x] Add first-pass animation graph/state machine.
- [x] Add required hockey locomotion state names to the graph contract.
- [x] Add required goalie animation state names to the graph contract.
- [x] Add required stick handling, stealing, shooting, and celebration state names to the graph contract.
- [ ] Add authored graph assets for those states.
- [x] Add hockey animation controller that maps gameplay state to animation state.
- [ ] Add renderer integration for animated/skinned meshes.
- [ ] Add network animation state hints once Phase 8 exists.
- [ ] Add editor preview support if needed.

## Started - Audio

- [x] Add audio settings.
- [x] Add audio backend/device.
- [x] Add audio clip asset/runtime type.
- [x] Add audio mixer.
- [x] Add audio buses.
- [x] Add audio source/emitter components.
- [ ] Add music playback support.
- [x] Add SFX playback support.
- [ ] Add spatial audio behavior.
- [ ] Add arena ambience.
- [x] Add first-pass puck hit sounds.
- [ ] Add skate sounds.
- [x] Add first-pass board/glass collision sounds.
- [ ] Add crowd reactions.
- [ ] Add goal horn.
- [x] Add first-pass UI sounds.
- [x] Add editor-editable hockey audio cue assignments.
- [x] Add editor playtest gameplay/physics audio routing.
- [ ] Add volume settings and persistence.
- [x] Add first-pass hockey audio controller and game-client gameplay/physics event routing to audio.

## Not Started - UI Framework And Screens

- [x] Add UI settings.
- [x] Add UI core primitives.
- [ ] Add UI layout/widget abstraction beyond RmlUi documents and direct element binding.
- [x] Add Vulkan-backed swapchain UI rendering path for runtime RmlUi overlay geometry/textures.
- [x] Add project-relative runtime UI image texture loading for RmlUi documents.
- [x] Add offscreen/editor Client Preview UI rendering path.
- [x] Add main menu RML document.
- [x] Add loading screen RML document.
- [x] Add lobby screen RML document with networking-unavailable state.
- [x] Add team/role select RML document.
- [x] Add settings screen RML document.
- [x] Add first match HUD RML document.
- [x] Add pause screen RML document.
- [x] Add scoreboard overlay RML document.
- [ ] Add network stats overlay.
- [ ] Add debug overlay.
- [x] Add end-match screen RML document.

## Not Started - Settings, Input, Accessibility

- [ ] Add configurable input bindings.
- [ ] Add rebinding UI.
- [ ] Add controller mappings.
- [ ] Add deadzone settings.
- [ ] Add sensitivity settings.
- [ ] Add vibration settings.
- [ ] Add input presets.
- [ ] Add graphics settings UI.
- [ ] Add audio settings UI.
- [ ] Add gameplay settings UI.
- [ ] Add network settings UI.
- [ ] Add accessibility options.
- [x] Provide high-quality code-owned runtime config fallbacks so packaged client/server/editor reset paths do not depend on editable config files for defaults.
- [ ] Persist user-facing runtime settings safely beyond the first sibling-TOML config path.

## Not Started - Client Flow And Integration

- [x] Add first-pass offline play flow through client-flow data and game-client UI actions.
- [ ] Add online play flow after Phase 8.
- [x] Connect first-pass gameplay snapshot state to HUD RmlUi elements.
- [ ] Connect lobby/team/role data to UI after Phase 8.
- [ ] Connect network stats to overlay after Phase 8.
- [x] Connect gameplay events to animation triggers.
- [x] Connect first-pass gameplay/physics client events to audio triggers.
- [ ] Connect network events to audio triggers after Phase 8.
- [ ] Ensure packaged client can launch into menus and start matches.

## Not Started - Profiling, Optimization, Packaging, QA

- [ ] Add CPU profiling support.
- [ ] Add GPU profiling support.
- [ ] Add network profiling support after Phase 8.
- [ ] Add memory tracking.
- [ ] Add asset memory budgets.
- [ ] Add graphics preset tuning.
- [ ] Add frame pacing review.
- [ ] Add loading-time optimization.
- [ ] Add server tick profiling.
- [ ] Add bandwidth optimization after Phase 8.
- [ ] Add performance overlay.
- [ ] Add Windows client package.
- [ ] Add Linux client package.
- [ ] Add Windows editor package.
- [ ] Add Linux editor package.
- [ ] Add Windows server package.
- [ ] Add Linux server package.
- [x] Add package scripts.
- [ ] Add runtime path rules for packaged builds.
- [ ] Add QA matrix.
- [ ] Add long-running server test.
- [ ] Add soak test script.

## Started / Partial

- [ ] First-pass animation target setup exists through `hockey_animation`, including CMake wiring, game-client/map-editor links, a dedicated-server headless guard, animation settings, skeleton/clip/pose data contracts, skeleton and clip validation, clip sampling, bind-pose fallback, joint hierarchy model-space pose building, skinning matrix construction, pose blending, animation-owned components, external component serialization hooks, first-pass graph/state-machine contracts, declaration-order transition evaluation, loop-aware animation event collection, registered in-memory graph runtime evaluation, ECS pose palette writes from loaded skeleton/clip assets, missing asset warning counters, disabled animation skip behavior, hockey gameplay snapshot/event-to-animation parameter mapping, and `HockeyAnimationTests` coverage for those contracts. Asset-side skeleton/animation structs, binary loaders, skinned mesh vertex data, cooked mesh/model version updates, legacy static mesh/model decode compatibility, AssetManager animation/skeleton loading, first-pass glTF skin/clip extraction, generated skeleton/clip descriptors, skeleton/animation cookers, metadata dependency wiring, model skeleton/animation references, ECS skinned mesh renderer bridge data, runtime pose palette data, and focused `HockeyAssetTests`/`HockeyECSTests` coverage also exist. Graph asset import/cook/load, renderer skinning, editor preview, runtime prefab wiring, broader real-art validation, and authored animation graph assets remain incomplete.
- [ ] First-pass audio system exists through `hockey_audio`, including settings, null/backend device setup, cooked audio assets, mixer/buses, source/listener components, editor audio preview, Project window Audio section, Project Settings cue assignment/preview, editor playtest gameplay/physics cue routing, game-client UI/gameplay/physics cue routing, and focused tests. Music/ambience/crowd loops, full spatial audio behavior, audio settings UI, packaging validation, and broader manual audio QA remain incomplete.
- [ ] Runtime RmlUi foundation exists through `hockey_ui`, including guarded file/system/render interfaces, project-relative UI texture loading, context lifetime, input mapping, click binding, shared polished RCSS with button hover/active/focus/disabled states, UI settings, and client-flow YAML; richer widget abstractions remain future work.
- [ ] First-pass RML/RCSS assets exist for home, loading, lobby, team select, match HUD, pause, settings, scoreboard, and end-match screens with first-pass polished layout classes across current runtime screens.
- [ ] Game client can initialize RmlUi from `[ui]`, boot the startup flow home screen, route basic button actions, load the flow's default offline gameplay scene on `StartOffline`, gate gameplay input while non-HUD UI screens are active, keep match HUD gameplay input live, draw RmlUi-generated overlay commands through the renderer swapchain UI pass, bind live first-pass HUD text, and bypass UI with `--no-ui`.
- [ ] Renderer UI overlay work uploads RmlUi geometry/textures to GPU resources and draws onto the swapchain or editor offscreen Client Preview target with `ui.vert`/`ui.frag`, premultiplied alpha, top-left scissor rectangles, transforms, and screenshot-visible ordering; broader visual validation still needs work.
- [ ] Match HUD RML exists and first-pass gameplay snapshot data is bound into live RmlUi elements; final presentation/notifications/network data remain incomplete.
- [ ] Editor Client Preview and client-flow authoring panels exist; Client Preview maps scaled preview clicks into RmlUi render-target coordinates and routes focused match-HUD gameplay input; Client Flow authoring validates screen/background/offline scene paths and Project panel opens RML/RCSS source files; manual UX polish and broader interaction validation remain.
- [ ] Package profiles, package command construction, `File > Package...`, Windows/Linux package scripts, and `just package-client` / `just package-server` exist. Current packages stage the app executable, sibling TOML copied from `data/config/editor.toml`, and app-local dynamic library dependencies, and generate `.hkpack` resource bundles; app targets now have high-quality code-owned config fallbacks, but they do not embed resource bundles yet and runtime UI/scene/tuning/asset/shader reads still use filesystem paths.
- [ ] No configurable bindings, controller presets, vibration, or input settings UI exists yet.
- [ ] No production profiling, embedded packaged-runtime loading, or long-running release QA harness exists yet.
- [ ] Visual polish systems such as ice reflections, skate spray, puck trails, crowd work, and goal presentation are not implemented.

## Left To Do

- [ ] Complete graph asset import/cook/load, renderer integration, editor preview, runtime prefab wiring, and broader real-art validation.
- [ ] Complete remaining audio polish: music, ambience, crowd, full spatial behavior, settings UI/persistence, packaging validation, and broader audio QA.
- [ ] Complete UI target, renderer, widgets, screens, HUD, overlays, and settings flows.
- [ ] Complete input rebinding, controller UX, accessibility, and persistence.
- [ ] Complete profiling, optimization, budget reporting, packaging, and QA/soak coverage.
- [ ] Verify Windows and Linux packaged client/editor/server builds.
- [ ] Verify local and online game flows after Phase 8 is complete.
- [ ] Preserve architecture boundaries and keep dedicated server headless.
