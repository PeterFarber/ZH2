# Phase 6 - Physics

Status: Implemented.

Source material:

- [x] Read `docs/phase-plans/phase-06-physics.md`.
- [x] Read `docs/phase-rules/100-phase-6-physics.mdc`.
- [x] Checked `docs/project-structure.md`.
- [x] Checked current source/CMake/test state.

## What This Phase Implements

- [x] `hockey_physics` Jolt-backed physics library.
- [x] Physics settings/config.
- [x] Physics initialization/shutdown.
- [x] Collision layers, layer filtering, and physics materials.
- [x] ECS physics components, serialization, metadata, and validation.
- [x] Physics body/shape abstractions and scene synchronization.
- [x] Contact events, trigger events, raycasts, overlaps, shape casts, and debug draw.
- [x] Client/editor/server physics integration.

## Finished - Target And Boundaries

- [x] `hockey_physics` target exists.
- [x] `HockeyPhysicsTests` target exists.
- [x] Physics depends on core and ECS.
- [x] Jolt is private to `hockey_physics`.
- [x] Public physics API does not expose Jolt types.
- [x] Physics does not link renderer, editor, gameplay, networking, Vulkan, or ImGui.
- [x] Server can use physics headlessly.

## Finished - Settings, Init, Layers, Materials

- [x] Physics settings load/save exists.
- [x] Fixed timestep config exists.
- [x] Gravity config exists.
- [x] Debug draw config exists.
- [x] Sleeping config exists.
- [x] Deterministic mode and integration substeps are applied.
- [x] Jolt allocator/factory/trace/assert initialization exists.
- [x] Physics shutdown path exists.
- [x] Collision layers exist for static, player, goalie, puck, rink, goal, trigger/sensor, and related hockey objects.
- [x] Collision filtering matrix exists.
- [x] Sensor layer collides with players/goalies/puck/stick and not static geometry.
- [x] Physics materials exist for hockey surfaces and bodies.
- [x] Friction and restitution combine behavior is applied at contacts.

## Finished - ECS Components And Shapes

- [x] `RigidBodyComponent` exists.
- [x] `PhysicsMaterialComponent` exists.
- [x] Box collider component exists.
- [x] Sphere collider component exists.
- [x] Capsule collider component exists.
- [x] Cylinder collider component exists.
- [x] Mesh collider component exists.
- [x] Trigger component exists.
- [x] Character controller component serializes as data.
- [x] Physics components serialize to YAML.
- [x] Physics components have editor metadata.
- [x] Collider validation exists.
- [x] Shape build validation exists.
- [x] Player/goalie body validation warns on missing/invalid capsule setup.

## Finished - World, Sync, Events, Queries

- [x] Physics world facade exists.
- [x] Scene-to-physics body creation and sync exist.
- [x] Physics-to-scene transform sync exists.
- [x] Fixed-step `PhysicsSystem` exists.
- [x] Edit/play/server mode behavior exists.
- [x] Contact event queue exists.
- [x] Trigger event queue exists.
- [x] Trigger detect flags are enforced.
- [x] Raycast exists.
- [x] Raycast-all exists.
- [x] Overlap sphere/box behavior exists.
- [x] Sphere shape cast exists.
- [x] Box shape cast exists.
- [x] Capsule shape cast exists.
- [x] Mesh collider runtime shape provider exists.
- [x] Editor installs an asset-backed mesh provider without making physics depend on assets.

## Finished - Debug, Editor, Server

- [x] Renderer-independent debug geometry is produced by physics.
- [x] Collider debug draw exists.
- [x] Trigger debug draw exists.
- [x] Contact point debug draw exists.
- [x] Editor physics preview exists.
- [x] Editor physics gizmo draws colliders and mesh collider placeholder boxes.
- [x] Hockey player tool spawns valid player/goalie capsule bodies.
- [x] Client `Play` mode steps physics.
- [x] Dedicated server `Server` mode steps physics.

## Finished - Tests And Verification

- [x] Physics initialization tests exist.
- [x] Collision layer tests exist.
- [x] Material tests exist.
- [x] Component serialization/metadata tests exist.
- [x] Collider validation tests exist.
- [x] Rigid body/world stepping tests exist.
- [x] Scene sync tests exist.
- [x] Contact/trigger tests exist.
- [x] Raycast and raycast-all tests exist.
- [x] Shape cast tests exist.
- [x] Substep/determinism tests exist.
- [x] Debug draw tests exist.
- [x] Headless server physics path is covered.

## Started / Partial

- [ ] `CharacterControllerComponent` serializes, but no Jolt character controller simulation exists.
- [ ] `SceneMode::ClientPrediction` is unused until Phase 8 networking/prediction work.
- [ ] Prediction-oriented rollback/replay behavior is not implemented.

## Left To Do

- [ ] Add Jolt `CharacterVirtual` or equivalent only if gameplay moves away from current rigid-body/custom movement.
- [ ] Add prediction-aware physics behavior during Phase 8 if network prediction requires it.
- [ ] Keep Jolt private and preserve renderer/editor/gameplay independence.
