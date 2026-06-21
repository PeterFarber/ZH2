# Hockey Arena Editor

Use this guide for rink layout, goals, boards, spawn points, collision geometry,
arena serialization, editor validation, and editor/runtime agreement.

## Goals

The arena editor and runtime should agree on the same arena data model. Avoid
editor-only changes that cannot be loaded or validated by the game runtime.

## Arena Concepts

Arena work may include:

- ice surface
- boards and glass
- goals and goal lines
- spawn points
- faceoff points
- collision geometry
- nav/AI helper regions
- camera markers
- lighting and reflection probes
- asset references

## Data Rules

When changing arena data:

1. Update serialization and deserialization together.
2. Preserve compatibility or document migration requirements.
3. Validate required objects such as goals and spawn points.
4. Keep collision data explicit and testable.
5. Avoid making gameplay depend on editor UI state.

## Validation Ideas

Useful checks include:

- required goals exist
- goal positions are valid
- spawn points are inside the playable area
- collision geometry is present
- asset references resolve
- IDs are unique
- coordinate units are consistent

## Completion Notes

Report:

- data format changes
- runtime load impact
- editor UI impact
- validation added or updated
- test arena used
