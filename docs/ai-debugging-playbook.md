# AI Debugging Playbook

Use this when a Codex or Cursor session needs to debug, visually verify, or
collect evidence for this project.

## Diagnostic Output

- Use `out/ai_diagnostics/` for AI-created bundles.
- Treat `_save/` as runtime/user output. Engine screenshots are still created
  under `_save/screenshots/`, but AI smoke scripts move only their own `ai_*`
  captures into `out/ai_diagnostics/<timestamp>/screenshots/`.
- Runtime logs are written under `data/logs/` unless a command uses `--log`.

## First Pass

- Reproduce with the narrowest command first.
- Inspect the latest relevant log before editing runtime code.
- Prefer bounded runs: `--max-frames` for client/editor and `--max-ticks` for
  the dedicated server.
- Prefer the closest subsystem test before the full suite.
- Report exact commands, exit codes, log paths, and screenshots used.

## Common Flows

Build failure:

- Fix the first real compiler or linker error.
- Check the owning `CMakeLists.txt` when a source/header was added or removed.
- Check architecture rules before adding target links or includes.

Test failure:

- Run the failing executable directly with `--root .`.
- Inspect the assertion/failure before broad refactors.
- Add or update focused tests only for behavior that changed.

Crash or runtime failure:

- Re-run with `--log out/ai_diagnostics/<timestamp>/logs/<app>.log` when useful.
- Inspect `data/logs/` and the command-specific log.
- Use bounded runs to make failures repeatable.

Renderer/editor/visual issue:

- Use a built-in screenshot command or AI visual smoke run.
- Do not claim visual correctness from source inspection alone.
- If a GPU/display is unavailable, report that limitation and the command for a
  follow-up visual run.

Physics/gameplay issue:

- Start with `HockeyPhysicsTests` or `HockeyGameplayTests`.
- Use the dedicated server bounded run for headless authoritative simulation.
- Check scene validation before changing simulation rules.

Asset/shader issue:

- Re-run `HockeyAssetTool validate` or `HockeyShaderTool` as appropriate.
- Check raw/cooked paths and shader source/bin paths.
- Do not edit generated cooked output unless the task is asset-pipeline work.

## Useful Commands

Windows text diagnostics:

```powershell
.\scripts\windows\ai_smoke.ps1 -Mode Text
```

Windows visual diagnostics:

```powershell
.\scripts\windows\ai_smoke.ps1 -Mode Visual
```

Windows full diagnostics:

```powershell
.\scripts\windows\ai_smoke.ps1 -Mode Full
```

Linux text diagnostics:

```bash
./scripts/linux/ai_smoke.sh --mode Text
```

Linux visual diagnostics:

```bash
./scripts/linux/ai_smoke.sh --mode Visual
```

## Report Template

Use this shape in final responses after debugging:

```text
Changed:
- <files/subsystems>

Verified:
- <command> -> <result>
- Logs/screenshots: <paths>

Not run:
- <command or check> because <reason>
```
