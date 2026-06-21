# Phase Plans

This folder contains detailed phase implementation plans.

The plans are large reference specs. They are useful when starting or revisiting
a phase-owned subsystem, but they should not be loaded by default for small
tasks.

## Authority

- `docs/phase-status/` is the current checklist for what is done, partial, or
  remaining.
- Source, CMake, tests, and runtime data are the implementation source of truth.
- `AGENTS.md` defines architecture and workflow rules.
- These plans are design/reference material. A checked-in plan does not mean the
  implementation currently exists.

## How To Use

1. Read `AGENTS.md`.
2. Read `docs/phase-status/README.md` and the matching phase status file.
3. Read the matching phase rule in `docs/phase-rules/`.
4. Read the matching phase plan only when the task needs detailed phase context.
5. Verify against current source, CMake, and tests before editing.

## Files

```text
phase-01-foundation-core.md
phase-02-ecs-scene-prefab.md
phase-03-vulkan-renderer.md
phase-04-unity-style-editor.md
phase-05-asset-pipeline.md
phase-06-physics.md
phase-07-hockey-gameplay.md
phase-08-networking-lobbies.md
phase-09-polish-animation-audio-ui.md
```
