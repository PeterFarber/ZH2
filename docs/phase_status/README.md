# Phase Status Guide

This folder is the AI-readable checklist for project phase state. Use it with
`docs/project-structure.md`, the matching `.cursor/plans/phase-*`
file, and current source/CMake truth.

## Phase Files

```text
phase-01-foundation-core.md          Implemented foundation/core status
phase-02-ecs-scene-prefab.md         Implemented ECS/scene/prefab status
phase-03-vulkan-renderer.md          Implemented renderer status and renderer gaps
phase-04-unity-style-editor.md       Implemented editor status and UI/GPU gaps
phase-05-asset-pipeline.md           Implemented asset pipeline status and format gaps
phase-06-physics.md                  Implemented physics status and prediction/controller gaps
phase-07-hockey-gameplay.md          Implemented local gameplay status and networking gaps
phase-08-networking-lobbies.md       Not-started networking/lobbies checklist
phase-09-polish-animation-audio-ui.md Not-started polish/animation/audio/UI checklist
```

## How To Read Checkboxes

- `[x]` means implemented or verified enough to treat as current repo state.
- `[ ]` under `Started / Partial` means known partial implementation or known
  verification gap.
- `[ ]` under `Not Started` means planned but not implemented.
- `[ ]` under `Left To Do` means work that remains before that phase or gap can
  be considered complete.
- `Finished Prerequisites From Earlier Phases` means supporting work exists, not
  that the current phase feature itself is implemented.

## Current Phase Interpretation

- Phases 1 through 7 are implemented in this checkout.
- Phase 8 is next but should only be implemented when explicitly requested.
- Phase 9 is future polish and should only be implemented when explicitly
  requested.
- Do not infer that a future phase is active because its status file exists.

## AI Update Rules

- Before editing a subsystem, read the matching phase status file.
- After every repo-tracked change, check whether phase status changed.
- Update the matching phase file in the same change when work completes an
  unchecked item, starts or partially implements an item, removes or invalidates
  existing work, changes verification status, or changes a known gap.
- If no phase status file needs to change, explicitly say
  "No phase status change needed" in the final response.
- Only mark an item `[x]` after the implementation and relevant verification are
  complete or after source/CMake truth proves it was already complete.
- If an item cannot be verified, keep it unchecked and add or preserve the
  verification gap.
- If source/CMake truth disagrees with this folder, trust source/CMake first and
  update the docs or call out the conflict.
