# AI Onboarding

Use this as the first-read guide for Cursor and Codex agents working in this
repository.

## Read First

1. `AGENTS.md` - hard architecture and development rules.
2. `docs/project-structure.md` - current target graph, repository layout,
   build/run commands, scripts, tests, and screenshot workflow.
3. `docs/phase_status/README.md` - how to interpret phase status checkboxes.
4. `docs/phase_status/` - per-phase checkbox status for finished, partial, and
   remaining work.
5. `docs/ai-debugging-playbook.md` - debugging, screenshots, and evidence
   collection workflow.
6. `.cursor/rules/` - Cursor-specific always-on and task-specific rules.
7. The relevant source, headers, CMake target, config, and tests for the task.

Prefer current source/CMake truth and `docs/project-structure.md`
over older summary text. If sources disagree, stop and call out the conflict.

## Current Project State

- Phases 1-7 are implemented.
- `hockey_gameplay` exists and is wired into the client, editor preview,
  dedicated server, and gameplay tests.
- `hockey_networking` does not exist yet.
- Phase 8 networking/lobbies is the next planned major subsystem, but only
  implement it when explicitly requested.

## Phase Status Rules

- `[x]` in `docs/phase_status/` means implemented or verified enough to treat as
  current repo state.
- `[ ]` under `Started / Partial` means known partial work or a verification gap.
- `[ ]` under `Not Started` or `Left To Do` means planned or remaining work, not
  current behavior.
- When source/CMake truth conflicts with phase status docs, trust source/CMake
  first and update the docs or call out the conflict.
- If a task completes a phase checklist item, update the matching phase status
  file in the same change.
- After every repo-tracked change, either update the matching phase status file
  or explicitly say "No phase status change needed" in the final response.

## Standard Commands

Windows first-time flow:

```powershell
.\scripts\windows\setup.ps1
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

Windows common runs:

```powershell
.\scripts\windows\run_client.ps1 --max-frames 5
.\scripts\windows\run_editor.ps1 --scene data/raw/scenes/main_rink.scene.yaml
.\scripts\windows\run_server.ps1 --max-ticks 300
.\scripts\windows\build_shaders.ps1
```

Windows AI diagnostics:

```powershell
.\scripts\windows\ai_smoke.ps1 -Mode Text
.\scripts\windows\ai_smoke.ps1 -Mode Visual
.\scripts\windows\ai_smoke.ps1 -Mode Full
```

Linux first-time flow:

```bash
./scripts/linux/setup.sh
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
```

Current Linux note: `scripts/linux/test.sh` only runs `HockeyCoreTests`. Run
other test executables directly from `build/linux-debug/apps/...` when needed.

Linux AI diagnostics:

```bash
./scripts/linux/ai_smoke.sh --mode Text
./scripts/linux/ai_smoke.sh --mode Visual
./scripts/linux/ai_smoke.sh --mode Full
```

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

On Windows, the full debug suite is:

```powershell
.\scripts\windows\test.ps1
```

## Editing Checklist

- Confirm the owning library/app before editing.
- Check dependency boundaries before adding includes or links.
- Update the owning `CMakeLists.txt` when adding/removing source files.
- Add or update focused tests for behavior changes.
- Check `docs/phase_status/` after code, config, build, script, or docs changes;
  update the matching phase file if phase state changed.
- Do not touch unrelated dirty files.
- Do not edit generated/cooked assets unless the task explicitly requires asset
  pipeline output.
- Keep Windows/Linux compatibility: use `std::filesystem`, CMake presets,
  PowerShell scripts for Windows, and Bash scripts for Linux.

## Common Boundaries

- `core` owns platform/runtime basics only.
- `ecs` owns entity/component/scene/prefab data only.
- `assets` owns CPU-side import/cook/load behavior only.
- `renderer` owns Vulkan rendering only and must not know hockey rules.
- `physics` owns physics simulation only and keeps Jolt private.
- `gameplay` owns hockey rules/simulation and must not directly use SDL3,
  Vulkan, or ImGui.
- `editor` can inspect/preview gameplay but does not own gameplay simulation.
- `dedicated_server` must stay headless and must not link renderer/editor/ImGui.

## Screenshots

Built-in screenshots write PNGs under:

```text
_save/screenshots/
```

AI-created diagnostic bundles belong under:

```text
out/ai_diagnostics/
```

Useful commands:

```powershell
.\scripts\windows\run_client.ps1 --screenshot gameplay --max-frames 5
.\scripts\windows\run_editor.ps1 --screenshot --screenshot-prefix editor_window --screenshot-frame 3 --max-frames 5
.\scripts\windows\run_editor.ps1 --capture-viewports --capture-prefix rink --capture-width 1920 --capture-height 1080 --max-frames 5
```

See `docs/project-structure.md` for the full screenshot mechanism and
flag reference.
