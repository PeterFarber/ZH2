# AI Tooling

This repo uses a small, explicit AI tool stack. Tools support the architecture
and phase rules in `AGENTS.md`; they do not replace source, CMake, tests, or
phase status.

## Command Surface

Use the root `justfile` first:

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

`just verify` is the normal completion contract for meaningful repo changes. If
it cannot run, report the exact missing tool, command, log, or environment
limitation.

## Serena

Use Serena for C++ semantic work:

- symbol declarations
- references
- diagnostics
- precise symbol-level edits
- impact checks before renames or shared API changes

Serena uses clangd for this project and works best after CMake configure has
produced a valid compile database.

Run:

```powershell
just serena-health
```

Sandboxed Codex runs may show false permission warnings when clangd or Python
tries to write outside the workspace. Rerun from a normal terminal before
changing config for a permission-only failure.

## Graphify

Use Graphify for broad architecture and impact analysis, especially before
large refactors or unfamiliar cross-subsystem work.

Good use cases:

- tracing input to shot execution
- understanding arena data into collision setup
- checking impact before changing snapshots or puck state
- onboarding to an unfamiliar subsystem

Generated output stays ignored under `graphify-out/`.

If community labels are generic, run:

```powershell
just graphify-name-communities
```

Do not use Graphify as a replacement for source/CMake truth, Serena references,
or tests.

## Beads

Use Beads for persistent task and blocker state:

- multi-session features
- bugs
- blockers
- dependency tracking
- follow-up work discovered during implementation

Do not create Beads entries for trivial edits, and do not let Beads rewrite
agent instructions.

If Beads must be initialized again:

```bash
bd init --skip-agents
```

## Superpowers And Local Skills

Use local skills when they match the task. They provide repeatable workflows for
debugging, planning, verification, Vulkan, shader work, hockey gameplay,
physics, networking, AI behavior, arena editing, and asset preparation.

Skills do not override `AGENTS.md`, project architecture, phase discipline, or
explicit user instructions.

## Dotagents

`agents.toml` is the dotagents manifest for Codex-oriented tool and skill
configuration. It does not replace `AGENTS.md`.

Validate it with:

```powershell
dotagents doctor
```

Do not add project skills just to mirror existing docs. Add a skill only for a
stable, repeatable workflow with concrete instructions that should travel with
the repo.

## Local-Only State

Do not commit generated local AI/tool state:

```text
.ai/
.agents/
.codex/
.cursor/
.beads/
.serena/
.graphifyignore
graphify-out/
.mcp.json
agents.lock
```

Generated logs, diagnostics, screenshots, build output, and caches stay local
unless the user explicitly asks to preserve an artifact.
