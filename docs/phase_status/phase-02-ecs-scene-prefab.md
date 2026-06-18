# Phase 2 - ECS, Scene, Prefab

Status: Implemented.

Source material:

- [x] Read `.cursor/plans/phase-02-ecs-scene-prefab.md`.
- [x] Checked `docs/project-structure-and-status.md`.
- [x] Checked `README.md` known-gap notes.

## What This Phase Implements

- [x] `hockey_ecs` data library.
- [x] Entity wrapper, scene system, scene modes, lifecycle, systems, and scene events.
- [x] Core authoring/game components plus hockey marker components.
- [x] Transform and hierarchy systems.
- [x] YAML scene/prefab serialization.
- [x] Scene validation.
- [x] Component metadata registry for tools and inspector UIs.
- [x] Prefab save/load, instancing, override tracking, apply, and revert.

## Finished - Target And Boundaries

- [x] `hockey_ecs` target exists and depends on `hockey_core`.
- [x] `HockeyECSTests` target exists.
- [x] Client, editor, and server can link ECS.
- [x] ECS remains data-oriented and does not own renderer, physics, gameplay, editor UI, SDL windowing, Vulkan, ImGui, or networking logic.

## Finished - Entities, Components, And Scene

- [x] Entity wrapper supports create/destroy.
- [x] Entity lookup by UUID and name exists.
- [x] Entity duplication exists.
- [x] Scene owns entity registry and lifecycle.
- [x] Scene modes exist: `Edit`, `Play`, `Server`, and `ClientPrediction`.
- [x] System interface exists for scene systems.
- [x] Scene event plumbing exists.
- [x] `Scene::Clear` behavior is covered.
- [x] Core authoring/game components are registered.
- [x] Hockey marker components exist for teams, players, goalie, puck, rink, goals, spawns, faceoffs, camera, and gameplay authoring.
- [x] Render-facing data components exist as plain ECS data.

## Finished - Transform, Hierarchy, And Active State

- [x] Transform component stores local transform data.
- [x] Parent and children hierarchy components exist.
- [x] Parent/child relationships update consistently.
- [x] Local/world transform propagation exists.
- [x] Active-in-hierarchy propagation exists.
- [x] Hierarchy metadata is registered as read-only/non-addable where appropriate.

## Finished - Serialization, Validation, Metadata

- [x] YAML helpers exist.
- [x] Component serialization helpers exist.
- [x] Scene YAML round-trip serialization exists.
- [x] Scene mode serializes with safe fallback.
- [x] Scene validation catches important hierarchy/UUID/component issues.
- [x] Component metadata registry supports editor inspection.
- [x] Metadata covers add/remove/edit rules used by the editor.

## Finished - Prefabs

- [x] Prefab serialization exists.
- [x] Prefab instancing remaps UUIDs.
- [x] Prefab instance links are tracked.
- [x] Prefab override set is owned by the scene and serialized.
- [x] Prefab diffing exists.
- [x] Apply instance-to-prefab support exists.
- [x] Revert instance-to-prefab support exists.
- [x] Prefab override persistence tests exist.

## Finished - Tests And Verification

- [x] ECS tests cover entity behavior.
- [x] ECS tests cover component behavior.
- [x] ECS tests cover transform/hierarchy behavior.
- [x] ECS tests cover scene serialization.
- [x] ECS tests cover scene validation.
- [x] ECS tests cover prefab behavior.
- [x] ECS tests cover metadata behavior.
- [x] ECS tests cover lifecycle and scene events.
- [x] ECS tests cover marker component round trips.
- [x] Client/editor/server scene integration exists.

## Started / Partial

- [ ] No open Phase 2-owned partial implementation is currently tracked.

## Left To Do

- [ ] Keep future ECS additions data-only.
- [ ] Do not move renderer, physics, gameplay, editor UI, SDL, Vulkan, ImGui, or networking behavior into `libs/ecs`.
- [ ] Re-check serialization, metadata, validation, and tests whenever new components are added.
