# AI Debugging

Use this playbook when debugging, visually verifying, or collecting evidence.

## Diagnostic Output

- Use `out/ai_diagnostics/` for AI-created bundles.
- Treat `_save/` as runtime/user output.
- Engine screenshots are created under `_save/screenshots/`.
- AI smoke scripts move only their own `ai_*` captures into
  `out/ai_diagnostics/<timestamp>/screenshots/`.
- Runtime logs are written under `data/logs/` unless a command uses `--log`.

## First Pass

- Reproduce with the narrowest command first.
- Inspect the latest relevant log before editing runtime code.
- Prefer bounded runs: `--max-frames` for client/editor and `--max-ticks` for
  the dedicated server.
- Prefer subsystem test executables before the full suite.
- Capture exact commands, exit codes, log paths, and screenshots used.

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
- If no GPU/display is available, report that limitation and give the follow-up
  command.

Physics/gameplay issue:

- Start with `HockeyPhysicsTests` or `HockeyGameplayTests`.
- Use a bounded dedicated server run for headless authoritative simulation.
- Check scene validation before changing simulation rules.

Asset/shader issue:

- Re-run `HockeyAssetTool validate` or `HockeyShaderTool` as appropriate.
- Check raw/cooked paths and shader source/bin paths.
- Do not edit generated cooked output unless the task is asset-pipeline work.

## Useful Commands

Windows:

```powershell
.\scripts\windows\ai_smoke.ps1 -Mode Text
.\scripts\windows\ai_smoke.ps1 -Mode Visual
.\scripts\windows\ai_smoke.ps1 -Mode Full
```

Linux:

```bash
./scripts/linux/ai_smoke.sh --mode Text
./scripts/linux/ai_smoke.sh --mode Visual
./scripts/linux/ai_smoke.sh --mode Full
```

## Report Shape

```text
Changed:
- <files/subsystems>

Verified:
- <command> -> <result>
- Logs/screenshots: <paths>

Not run:
- <command or check> because <reason>
```
