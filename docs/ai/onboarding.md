# AI Onboarding

Use this as the first-read checklist for AI sessions that will change the repo.

## Start Here

1. Read `AGENTS.md`.
2. Identify the owning library, app, tool, script, or documentation area.
3. Read `docs/project-structure.md` when target ownership, app flags, scripts,
   screenshots, or generated output paths matter.
4. Read `docs/phase-status/README.md` and the phase status file that owns the
   subsystem.
5. For phase-owned work, read the matching compact rule in `docs/phase-rules/`.
6. Read the matching detailed plan in `docs/phase-plans/` only when the task
   needs detailed phase context.
7. Read the relevant source, headers, CMake target, config, tests, and nearby
   docs before editing.
8. Use a focused guide from `docs/ai/hockey/` only when the task matches it.

Prefer current source/CMake truth over summary text. If sources disagree, stop
and call out the conflict before making a risky change.

## Phase Selection

Do not assume a phase from the docs file you happen to have open. Determine the
active phase from the user's request, the owning subsystem, and
`docs/phase-status/`.

Use `docs/phase-status/README.md` for checkbox semantics and update rules.
Future-phase systems require an explicit user request.

## Commands

Prefer the root `justfile`:

```powershell
just tools-check
just serena-health
just ai-ready
just configure-debug
just build-debug
just test
just smoke-text
just verify
```

Use direct platform scripts when `just` is unavailable. See
`docs/project-structure.md` for script paths and app flags.

## Narrow Test Targets

Use the narrowest relevant executable after source changes:

```text
HockeyCoreTests
HockeyECSTests
HockeyAssetTests
HockeyRendererTests
HockeyEditorTests
HockeyPhysicsTests
HockeyGameplayTests
```

Use `just test` for the full debug suite.

## Editing Checklist

- Confirm the owning library/app before editing.
- Check dependency boundaries before adding includes or links.
- Update the owning `CMakeLists.txt` when adding or removing source files.
- Add or update focused tests for behavior changes.
- Do not touch unrelated dirty files.
- Do not edit generated/cooked assets unless explicitly required.
- Keep Windows/Linux compatibility.
- Follow `docs/phase-status/README.md` before finalizing.

## Common Boundaries

- `core` owns platform/runtime basics only.
- `ecs` owns entity/component/scene/prefab data only.
- `assets` owns CPU-side import/cook/load behavior only.
- `renderer` owns Vulkan rendering only and must not know hockey rules.
- `physics` owns physics simulation and keeps Jolt private.
- `gameplay` owns hockey rules/simulation and must not directly use SDL3,
  Vulkan, or ImGui.
- `editor` can inspect/preview gameplay but does not own gameplay simulation.
- `dedicated_server` must stay headless and must not link renderer/editor/ImGui.
