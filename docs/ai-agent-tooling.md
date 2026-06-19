# AI Agent Tooling

This repo uses a small, explicit Codex tool stack. The tools support the
existing architecture and phase rules; they do not replace them.

`AGENTS.md` remains the authoritative project rule file. `agents.toml` is the
dotagents manifest for Codex-oriented tool and skill configuration; it does not
replace `AGENTS.md`.

Source, CMake, tests, and `docs/phase_status/` remain the source of truth for
implementation state. The `docs/ai-rules/` and `docs/phase-plans/` directories
remain useful shared project guidance: current AI sessions should read the
relevant files there as supporting context, not as separate sources of truth.

## AI Session Loop

Use this loop for efficient AI-assisted coding:

1. Orient on `AGENTS.md`, `docs/ai-onboarding.md`, the relevant phase status
   file, and the owning source/CMake/tests.
2. Check local tooling with `just tools-check` or `just ai-ready` when the task
   depends on semantic tools.
3. Use `rg` and direct file reads for exact source truth.
4. Use Serena for symbol lookup, references, diagnostics, and semantic edits.
5. Use Graphify before broad architecture questions, unfamiliar subsystems, or
   high-blast-radius refactors.
6. Use Beads only for persistent task/blocker tracking; do not let it rewrite
   agent instructions.
7. Finish with the narrowest meaningful verification, then `just verify` when
   the change warrants the full contract.

## Command Surface

Use the root `justfile` as the first choice for common project commands:

```text
just tools-check
just serena-health
just ai-ready
just configure-debug
just build-debug
just test
just smoke-text
just verify
```

`just verify` is the normal completion contract for repo changes. If it cannot
run, report the exact missing tool, command, log, or environment limitation.

The existing platform scripts remain supported and are what the `justfile`
wraps.

## Always-Useful Tools

- `just`: shared command runner for humans and Codex.
- `dotagents`: one committed `agents.toml` for project-level agent config
  hygiene. Generated state stays local.
- Serena: primary semantic code intelligence for C++ symbol lookup, references,
  diagnostics, and semantic editing.
- Superpowers: process guidance for unclear features, gameplay/physics work,
  architectural changes, networking, replay/serialization, and difficult bugs.
- Beads: persistent task/blocker tracking only.

Beads must be initialized without taking ownership of repo instructions:

```bash
bd init --skip-agents
```

## Dotagents Skills

Do not add `.agents` skills just because `.agents/skills/` exists. Keep
`agents.toml` skill-free until there is a concrete, repeatable workflow that
benefits from a versioned skill.

Good candidates for future skills:

- phase closeout and verification
- gameplay replay/debug capture
- renderer visual smoke triage
- asset import/cook validation

Existing project rules belong in `AGENTS.md`, `docs/ai-rules/`, and
`docs/phase-plans/`; duplicating them as skills would create another source of
truth.

## Serena

Serena is registered as a Codex MCP server:

```text
serena start-mcp-server --context=codex --project-from-cwd
```

The tracked `.serena/project.yml` keeps the portable C++ project definition and
uses Serena's `cpp` language backend, which runs clangd. Local
machine-specific overrides can still live in ignored `.serena/project.local.yml`
when needed, but the default project flow should use clangd directly.

Run Serena health checks with UTF-8 console output on Windows:

```powershell
just serena-health
```

The `just` recipe summarizes successful health checks and leaves the full log in
`.serena/logs/`. Run it from a normal terminal for the cleanest signal. A
sandboxed Codex tool run may produce false `AppData\Local\clangd\index`
permission warnings or a Serena/joblib `WinError 5` in the full log because the
sandbox blocks writes and Windows pipe creation outside the workspace.

If clangd starts crashing again, first check `.clang-tidy` and workspace clangd
arguments before adding a local language-server override.

## On-Demand Tools

Graphify is useful before broad architecture questions or large refactors, but
it should stay manual and generated output should remain ignored.

Good Graphify use cases:

- tracing player input to shot execution
- understanding arena geometry into collision setup
- checking impact before changing gameplay snapshots or puck state
- onboarding to an unfamiliar subsystem

If Graphify leaves community labels as `Community N`, run the local offline
namer:

```powershell
just graphify-name-communities
```

Graphify's native `graphify label .` command can produce LLM-backed labels when
a backend/API key is configured. The `just` recipe is deterministic and works
without API access by deriving names from dominant source paths and symbols.

When regenerating the report and large force-graph HTML locally, use:

```powershell
$env:GRAPHIFY_VIZ_NODE_LIMIT = "10000"
graphify cluster-only . --graph graphify-out/graph.json --no-label
just graphify-name-communities
```

Avoid automatic Graphify commit hooks until the generated graph is proven useful
and low-noise for this project.

## Deferred Tools

Do not add these in the first tooling pass:

- `just-mcp`
- CTest/GoogleTest migration
- default sccache presets
- SwiftShader CI smoke tests
- CodeChecker/reviewdog integration
- deterministic replay/hash runner
- dmux or other parallel worktree orchestration

These are good future options, but the current repo already has direct test
executables, cross-platform scripts, headless server runs, and gameplay tests.

## Local Setup Notes

Suggested local install order:

1. Install `just` and run `just --list`.
2. Install `dotagents`, then run `dotagents install` or `dotagents doctor`.
3. Install Serena, run `serena init`, configure Codex, and run a health check
   after a successful debug configure with `just serena-health`.
4. Install Superpowers through the Codex plugin UI.
5. Install Beads and run `bd init --skip-agents`.
6. Install Graphify, run it manually when useful, and keep `graphify-out/`
   ignored.

Use `just tools-check` for a quick local PATH inventory and `just ai-ready` to
confirm the basic AI coding toolchain.
