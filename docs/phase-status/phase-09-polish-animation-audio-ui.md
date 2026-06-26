# Phase 9 - Polish, Animation, Audio, UI

Status: Started for runtime RmlUi UI foundation. Animation, audio, editor Client Preview, full HUD data binding, and packaging remain incomplete.

Source material:

- [x] Read `docs/phase-plans/phase-09-polish-animation-audio-ui.md`.
- [x] Read `docs/phase-rules/130-phase-9-polish-animation-audio-ui.mdc`.
- [x] Checked current source/CMake/test state.
- [x] Checked `docs/project-structure.md`.

## What This Phase Implements

- [ ] `hockey_animation` target.
- [ ] `hockey_audio` target.
- [x] `hockey_ui` target.
- [x] `HockeyUITests` covers runtime UI foundation behavior.
- [ ] Animation clips, skeletons, poses, blending, state machines, and hockey animation controller.
- [ ] Audio device, mixer, buses, clips, sources, and hockey audio event controller.
- [ ] UI framework, UI rendering, widgets, screens, HUD, overlays, and settings.
- [ ] Input rebinding, controller UX, accessibility, profiling, optimization, packaging, QA, and soak testing.

## Finished Prerequisites From Earlier Phases

- [x] Renderer has material/settings names for some future hockey visuals.
- [x] Renderer can draw the game scene and editor viewports.
- [x] Renderer screenshot support exists for visual verification.
- [x] Asset type placeholders exist for audio.
- [x] Asset type placeholders exist for animation.
- [x] Editor UI exists, but it is not the final in-game UI system.
- [x] Gameplay exposes match, player, puck, score, clock, goal, and event state for future HUD/audio/animation systems.
- [x] Networking is planned to provide lobby/team/role/network stats data before final UI polish is complete.

## Not Started - Target And Dependency Setup

- [ ] Create `libs/animation/`.
- [ ] Create `libs/audio/`.
- [x] Create `libs/ui/`.
- [ ] Add CMake targets for animation and audio.
- [x] Add CMake target for UI.
- [ ] Add app link updates for editor UI preview where appropriate.
- [x] Link the game client to `hockey_ui`.
- [ ] Add package targets/scripts.
- [ ] Add tests for animation/audio/polish systems.
- [x] Add tests for UI settings, client-flow assets, RmlUi interfaces/context/input mapping, and architecture guards.
- [ ] Keep dedicated server free of renderer, editor UI, game UI rendering, and audio playback.

## Not Started - Animation

- [ ] Add animation settings.
- [ ] Add skeleton representation.
- [ ] Add joint hierarchy support.
- [ ] Add animation clip representation.
- [ ] Add pose data.
- [ ] Add clip sampling.
- [ ] Add pose blending.
- [ ] Add animation components.
- [ ] Add animation graph/state machine.
- [ ] Add hockey locomotion states.
- [ ] Add goalie animation states.
- [ ] Add stick handling, stealing, shooting, and celebration states.
- [ ] Add hockey animation controller that maps gameplay state to animation state.
- [ ] Add renderer integration for animated/skinned meshes.
- [ ] Add network animation state hints once Phase 8 exists.
- [ ] Add editor preview support if needed.

## Not Started - Audio

- [ ] Add audio settings.
- [ ] Add audio backend/device.
- [ ] Add audio clip asset/runtime type.
- [ ] Add audio mixer.
- [ ] Add audio buses.
- [ ] Add audio source/emitter components.
- [ ] Add music playback support.
- [ ] Add SFX playback support.
- [ ] Add spatial audio behavior.
- [ ] Add arena ambience.
- [ ] Add puck hit sounds.
- [ ] Add skate sounds.
- [ ] Add board/glass collision sounds.
- [ ] Add crowd reactions.
- [ ] Add goal horn.
- [ ] Add UI sounds.
- [ ] Add volume settings and persistence.
- [ ] Add hockey audio controller that maps gameplay/network events to audio.

## Not Started - UI Framework And Screens

- [x] Add UI settings.
- [x] Add UI core primitives.
- [ ] Add UI layout/widgets.
- [ ] Add full Vulkan-backed UI rendering path.
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
- [ ] Persist settings safely.

## Not Started - Client Flow And Integration

- [x] Add first-pass offline play flow through client-flow data and game-client UI actions.
- [ ] Add online play flow after Phase 8.
- [ ] Connect gameplay state to HUD.
- [ ] Connect lobby/team/role data to UI after Phase 8.
- [ ] Connect network stats to overlay after Phase 8.
- [ ] Connect gameplay events to animation triggers.
- [ ] Connect gameplay/network events to audio triggers.
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
- [ ] Add package scripts.
- [ ] Add runtime path rules for packaged builds.
- [ ] Add QA matrix.
- [ ] Add long-running server test.
- [ ] Add soak test script.

## Started / Partial

- [ ] No animation library/system exists yet.
- [ ] No audio library/system exists yet.
- [ ] Runtime RmlUi foundation exists through `hockey_ui`, including guarded file/system/render interfaces, context lifetime, input mapping, click binding, UI settings, and client-flow YAML.
- [ ] First-pass RML/RCSS assets exist for home, loading, lobby, team select, match HUD, pause, settings, scoreboard, and end-match screens.
- [ ] Game client can initialize RmlUi from `[ui]`, boot the startup flow home screen, route basic button actions, render RmlUi-generated overlay commands, and bypass UI with `--no-ui`.
- [ ] Renderer UI overlay work is currently a generic handle/draw-command API and CPU-side command capture; Vulkan swapchain/offscreen UI drawing, UI shaders, alpha/scissor pipeline state, and screenshot-visible UI still need implementation.
- [ ] Match HUD RML exists, but gameplay snapshot data is not yet bound into live RmlUi elements.
- [ ] Editor Client Preview and client-flow authoring panels are not implemented yet.
- [ ] No configurable bindings, controller presets, vibration, or input settings UI exists yet.
- [ ] No production profiling, packaging, or long-running release QA harness exists yet.
- [ ] Visual polish systems such as ice reflections, skate spray, puck trails, crowd work, and goal presentation are not implemented.

## Left To Do

- [ ] Complete animation target, data model, runtime sampling, blending, graph, controller, and renderer integration.
- [ ] Complete audio target, backend, mixer, buses, clips, emitters, settings, and hockey event integration.
- [ ] Complete UI target, renderer, widgets, screens, HUD, overlays, and settings flows.
- [ ] Complete input rebinding, controller UX, accessibility, and persistence.
- [ ] Complete profiling, optimization, budget reporting, packaging, and QA/soak coverage.
- [ ] Verify Windows and Linux packaged client/editor/server builds.
- [ ] Verify local and online game flows after Phase 8 is complete.
- [ ] Preserve architecture boundaries and keep dedicated server headless.
