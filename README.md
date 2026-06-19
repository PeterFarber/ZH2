# HockeyGame

Custom C++20 hockey game and engine project targeting a real-time 4v4 hockey experience with a Vulkan renderer, SDL3 platform layer, Unity-style editor, ECS scene/prefab workflow, physics, gameplay simulation, and an eventual authoritative networking stack.

Current status: phases 1-7 are substantially implemented, covering foundation/core through local and headless hockey gameplay. Phase 8 networking/lobbies and Phase 9 animation/audio/UI/polish are still future work.

## Start Here

- [Project structure](docs/project-structure.md) - module layout, target graph, app list, scripts, build/run commands, and screenshot workflow.
- [Phase status index](docs/phase_status/README.md) - per-phase checkbox status files with finished, started, and remaining work.
- [Agent instructions](AGENTS.md) - architecture boundaries, dependency rules, phase order, and coding expectations.
- [AI onboarding](docs/ai-onboarding.md) - how AI agents should orient themselves before making changes.
- [AI workflow guide](docs/ai-workflow-guide.md) - day-to-day instructions for using Codex and local AI tools.
- [AI agent tooling](docs/ai-agent-tooling.md) - shared command surface and optional agent tool policy.
- [AI debugging playbook](docs/ai-debugging-playbook.md) - build/test/screenshot/debug workflow for AI-assisted development.
- [AI rules](docs/ai-rules/), [phase plans](docs/phase-plans/), and [phase rules](docs/phase-rules/) - AI workflow and phase-specific guidance.

## Quick Build

Dependencies are managed with vcpkg manifest mode through [vcpkg.json](vcpkg.json). Set `VCPKG_ROOT` to your vcpkg install before configuring.

Preferred command surface when `just` is installed:

```powershell
just tools-check
just ai-ready
just configure-debug
just build-debug
just test
just verify
```

Direct platform scripts remain supported.

Windows:

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

Linux:

```bash
./scripts/linux/setup.sh
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
```

Run helpers:

```powershell
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_client.ps1
.\scripts\windows\run_server.ps1
```

Linux equivalents live in [scripts/linux](scripts/linux).

## Repository Map

```text
apps/       Executables: client, editor, server, tools, tests
libs/       Engine libraries: core, ecs, assets, renderer, physics, gameplay, editor
data/       Config, raw/cooked assets, shaders, editor state
scripts/    Windows and Linux build/run/test helpers
docs/       Project documentation, phase status, AI workflow docs
  ai-rules/     Shared AI project rules
  phase-plans/  Detailed phase implementation plans
justfile    Shared build/test/run/verify command recipes
agents.toml Shared Codex agent-tool declarations
```

For the detailed dependency graph and subsystem notes, use [docs/project-structure.md](docs/project-structure.md).

## Core Targets

- `HockeyGameClient` - playable windowed client path.
- `HockeyMapEditor` - Unity-style map editor and authoring workflow.
- `HockeyDedicatedServer` - headless authoritative server path.
- `HockeyAssetTool` - asset discovery/import/cook/validate CLI.
- Test apps cover core, ECS, renderer, editor, assets, physics, and gameplay.

## Development Model

Development is phase-based. Each subsystem should be completed in order and architecture boundaries should stay clean. The short version:

1. Foundation/core
2. ECS, scene, prefab
3. Vulkan renderer
4. Unity-style editor
5. Asset pipeline
6. Physics
7. Hockey gameplay
8. Networking/lobbies
9. Animation/audio/UI/polish

Use [docs/phase_status](docs/phase_status) for current phase details before starting work.

## License

[MIT License](LICENSE) - Copyright (c) 2026 Peter Farber.
