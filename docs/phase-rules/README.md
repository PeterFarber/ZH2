# Phase Rules

This folder contains compact phase-specific rule sets.

Use these files when changing a subsystem owned by a phase or when starting a
future phase. They are shorter than the detailed plans in `docs/phase-plans/`
and are useful for quick scope and boundary checks.

## Authority

- `AGENTS.md` defines the hard project rules.
- `docs/phase-status/` defines current implementation status.
- Source, CMake, tests, and runtime data win when docs disagree.
- These phase rules describe intended scope and constraints. They do not prove
  that code exists.

## Files

```text
050-phase-1-foundation-core.mdc
060-phase-2-ecs-scene-prefab.mdc
070-phase-3-vulkan-renderer.mdc
080-phase-4-unity-style-editor.mdc
090-phase-5-asset-pipeline.mdc
100-phase-6-physics.mdc
110-phase-7-hockey-gameplay.mdc
120-phase-8-networking-lobbies.mdc
130-phase-9-polish-animation-audio-ui.mdc
```

Phase 8 and Phase 9 are future work. Do not implement them unless explicitly
requested.
