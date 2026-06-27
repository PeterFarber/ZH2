# Phase 7 - Hockey Gameplay

Status: Implemented for local client, editor preview, and headless server use, aligned to the `_save/docs/GAMEPLAY.md` steal/shoot-only action model.

Source material:

- [x] Read `docs/phase-plans/phase-07-hockey-gameplay.md`.
- [x] Read `docs/phase-rules/110-phase-7-hockey-gameplay.mdc`.
- [x] Reviewed `GAMEPLAY.md` target action design.
- [x] Checked `docs/project-structure.md`.
- [x] Checked current source/CMake/test state.

## What This Phase Implements

- [x] `hockey_gameplay` hockey rules/simulation library.
- [x] Gameplay settings and tuning.
- [x] Input model, input buffering, and gameplay event queue.
- [x] 4v4 match setup with 3 skaters plus 1 goalie per team.
- [x] Match, team, player, goalie, puck, stick, possession, goal, score, period, faceoff, reset, and out-of-play systems.
- [x] Gameplay snapshots for future replication.
- [x] Editor validation/preview, client local gameplay, and server headless gameplay integration.

## Finished - Target And Boundaries

- [x] `hockey_gameplay` target exists.
- [x] `HockeyGameplayTests` target exists.
- [x] Gameplay depends on core, ECS, and physics.
- [x] Gameplay does not directly depend on renderer, editor UI, SDL3, Vulkan, ImGui, or networking.
- [x] Gameplay owns hockey rules; editor/client/server integrate or preview those rules.

## Finished - Settings, Tuning, Input, Events

- [x] Gameplay settings load from config.
- [x] Gameplay rules/tuning YAML exists.
- [x] Team types exist.
- [x] Role types exist.
- [x] Player slot assignment types exist.
- [x] Gameplay input frame exists.
- [x] Input buffering exists.
- [x] Gameplay event queue exists.
- [x] Gameplay event names exist.
- [x] Countdown, steal, boost, and goalie shield gameplay event names exist.
- [x] Shot grace, puck floor height, and shot charge tuning exists.
- [x] Gameplay input exposes click-only steal and shot press/hold/release, with no pass, body-check, poke-check, steal-held, or steal-released action fields.
- [x] Gameplay settings, tuning, config, and editor controls expose no pass, body-check, or poke-check controls.
- [x] Gameplay tuning YAML is consumed by editor preview, client local play, and headless server runtime.
- [x] Gameplay settings expose a selectable waypoint marker prefab path.
- [x] Optional gameplay event logging exists.

## Finished - Components And Validation

- [x] Match state component exists.
- [x] Score state component exists.
- [x] Team state component exists.
- [x] Player component exists.
- [x] Skater component exists.
- [x] Goalie component exists.
- [x] Puck component exists.
- [x] Possession component exists.
- [x] Stick component exists.
- [x] Goal component exists.
- [x] Out-of-play component exists.
- [x] Faceoff component exists.
- [x] Gameplay components serialize to scene YAML.
- [x] Gameplay components have editor metadata.
- [x] Gameplay-ready scene validation exists.
- [x] Main rink gameplay validation exists.
- [x] Hockey player authoring creates role-specific solid skater and goalie capsule bodies.

## Finished - Match And Rules

- [x] Match initialization exists.
- [x] Player spawn/setup flow uses deterministic shuffled team spawn pools.
- [x] Match setup attaches configured stick prefabs to spawned player prefabs while preserving root stick gameplay data.
- [x] 4v4 setup exists.
- [x] Match clock exists.
- [x] Period clock behavior exists.
- [x] 3x180-second period defaults exist.
- [x] Pregame countdown exists.
- [x] Countdown tick/beep gameplay events exist.
- [x] Faceoff flow exists.
- [x] Reset flow uses neutral/Home/Away faceoff spawn pools with deterministic placement.
- [x] Score system exists.
- [x] Goal detection exists.
- [x] Out-of-play handling exists.
- [x] Skater movement exists.
- [x] Skater impulse boost exists.
- [x] Brake clears waypoints and double-tap stops skaters.
- [x] Goalie movement support exists.
- [x] Goalie two-charge boost exists.
- [x] Goalie shield reflects pucks and bounces players.
- [x] Puck gameplay state exists.
- [x] Puck controller exists.
- [x] Puck possession exists.
- [x] Loose and eligible shot pucks can be acquired by stick range or player-body collider contact.
- [x] Body-contact puck acquisition respects authored collider transform scale.
- [x] Local Main scene gameplay regression asserts continuous puck collision detection when `Main.scene.yaml` contains the authored puck/goals fixture.
- [x] Possessed dynamic pucks drive their physics body transform so physics sync does not undo pickup.
- [x] Possessed pucks from pickup or steal preserve stick X/Z while locking Y to the tuned puck floor height.
- [x] Stick handling exists.
- [x] Shooting exists.
- [x] Shot release pushes velocity into dynamic puck physics bodies so physics sync does not keep the puck at the stick.
- [x] Shot release has a tuned shooter self-collision/reacquire grace window.
- [x] Puck controller clamps puck height to the tuned ice/floor height.
- [x] Explicit steal action exists.
- [x] Successful steals directly transfer possession from the current possessor to the stealing player.
- [x] Legacy passing, body-check, and poke-check gameplay systems are removed.
- [x] Contextual left-click steal-or-shot mapping exists in local client/editor preview input translation.
- [x] Successful steals require the steal click to be released before the same player can charge or release a shot.
- [x] Local client/editor preview shots aim toward the mouse cursor projected onto the ice.
- [x] Click-to-move waypoint targets instantiate and reuse configured prefab markers while move targets are active.

## Finished - World, Snapshots, Integration

- [x] `GameplayWorld` orchestrates scene, physics, input, events, and tuning on a fixed step.
- [x] Local gameplay player and goalie bodies are dynamic, gravity-free physics actors driven by gameplay velocity while physics owns movement and board response.
- [x] Gameplay system/world ticking exists.
- [x] Match snapshots exist.
- [x] Player snapshots exist.
- [x] Player snapshots expose shot charge state/ratio.
- [x] Puck snapshots exist.
- [x] Client local gameplay integration exists.
- [x] Client local play draws a lightweight shot power bar while charging.
- [x] Editor gameplay validation integration exists.
- [x] Editor gameplay preview integration exists.
- [x] Dedicated server headless gameplay integration exists.
- [x] Reset match hotkey/path exists in local play.

## Finished - Tests And Verification

- [x] Gameplay settings/tuning tests exist.
- [x] Input tests exist.
- [x] Event tests exist.
- [x] Team/component tests exist.
- [x] Match setup tests exist.
- [x] Gameplay world tick tests exist.
- [x] Reset tests exist.
- [x] Player movement tests exist.
- [x] Puck interaction/controller tests exist.
- [x] Puck interaction tests cover player-body contact possession.
- [x] Puck interaction tests cover scaled puck/player body-contact possession.
- [x] Shooting tests exist.
- [x] Shooting tests cover dynamic puck physics body velocity on shot release.
- [x] Shooting tests cover post-shot shooter grace.
- [x] Puck controller tests cover floor-height clamping.
- [x] Snapshot tests cover shot charge ratio.
- [x] Steal tests cover direct possession transfer, cooldown, attempt events, failed steals, and no loose-puck poke behavior.
- [x] Steal tests cover blocking shot charge/release until after the steal click is released.
- [x] Gameplay/editor tests cover the absence of pass, body-check, poke-check, and steal held/released action surfaces.
- [x] Goal tests exist.
- [x] Goal trigger tests cover puck-only scoring.
- [x] Out-of-play tests exist.
- [x] Snapshot tests exist.
- [x] Gameplay scene regression prefers `data/raw/scenes/Main.scene.yaml` and falls back to the tracked main rink fixture when Main lacks gameplay markers.
- [x] Headless server gameplay path is covered.
- [x] Editor Hockey Players tool tests cover role-specific skater and goalie collider authoring.

## Started / Partial

- [ ] Gameplay snapshots exist locally, but are not replicated over a network transport.
- [ ] Input model exists locally, but input streaming over the network is not implemented.
- [ ] `SceneMode::ClientPrediction` exists, but prediction/reconciliation are not implemented.
- [ ] Client gameplay is local play, not networked multiplayer.
- [ ] Gameplay events exist locally, but reliable network event replication is not implemented.

## Left To Do

- [ ] Connect input frames and snapshots to Phase 8 networking.
- [ ] Add server-side network input validation during Phase 8.
- [ ] Add snapshot interpolation, prediction, and reconciliation during Phase 8.
- [ ] Replicate puck, player, match, goal, score, period, and event state during Phase 8.
- [ ] Add animation/audio/UI polish around gameplay during Phase 9.
