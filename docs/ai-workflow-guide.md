# Using AI On This Project

This project is ready for Codex-assisted development when the local readiness
check passes:

```powershell
just ai-ready
```

That check confirms the shared command surface, local AI tools, and Serena's
clangd-backed semantic tooling. It does not replace build and test verification;
use `just verify` for the normal completion contract after meaningful repo
changes.

## Quick Start

From the repo root:

```powershell
just tools-check
just ai-ready
```

Before asking Codex to change code, make sure the task points at the relevant
subsystem, phase, or bug. A good prompt is specific about ownership and
verification:

```text
Use AGENTS.md and the phase status docs. Implement <feature/fix> in
<library/app>, preserve the dependency boundaries, add or update focused tests,
and run the narrowest useful verification.
```

For larger changes, ask Codex to inspect these first:

```text
AGENTS.md
docs/ai-onboarding.md
docs/phase_status/
docs/project-structure.md
the owning source, headers, CMakeLists.txt, and tests
```

## What Each Piece Does

`AGENTS.md` is the authoritative rule file. It defines the architecture
boundaries, phase order, dependency direction, coding expectations, and the
requirement to check `docs/phase_status/` after repo-tracked changes.

`agents.toml` is the dotagents manifest for Codex-oriented tool configuration.
It does not replace `AGENTS.md`, and it should stay small until the project has
repeatable workflows worth turning into skills.

`docs/ai-onboarding.md` is the first-read checklist for AI sessions. It tells
Codex what to read before editing and how to choose verification.

`docs/ai-agent-tooling.md` explains the installed AI tool stack and tool policy.
Use it when deciding whether to use Serena, Graphify, Beads, Superpowers, or
dotagents.

`docs/ai-rules/` contains the project AI guidance that used to live in Cursor
rules. Treat it as supporting context for the current Codex flow.

`docs/phase-plans/` contains detailed phase implementation plans. These are
planning context, not proof that code exists.

`docs/phase_status/` is the current checklist for what is done, partial, or not
started. If source/CMake truth disagrees with a phase status file, trust the
source first and update or call out the doc mismatch.

`justfile` is the shared command surface for humans and Codex. Prefer it over
remembering individual script paths.

## Daily AI Workflow

1. Read the task, then identify the owning library or app.
2. Read `AGENTS.md`, the relevant phase status file, and nearby source/CMake/tests.
3. Use `rg` for exact text and file discovery.
4. Use Serena for symbol lookup, references, diagnostics, and semantic context.
5. Use Graphify before broad architecture questions or high-blast-radius refactors.
6. Make focused changes inside the owning boundary.
7. Run the narrowest useful tests first.
8. Run `just verify` when the change warrants the full local contract.
9. Update `docs/phase_status/` if phase state changed. Otherwise, say
   "No phase status change needed" in the final response.

## Commands To Know

Tool readiness:

```powershell
just tools-check
just serena-health
just ai-ready
```

Build and test:

```powershell
just configure-debug
just build-debug
just test
just verify
```

Bounded smoke runs:

```powershell
just smoke-text
just smoke-visual
just smoke-full
```

App launch helpers:

```powershell
just run-client
just run-editor
just run-server
```

## Serena And Clangd

Serena is the semantic code intelligence layer. It uses clangd for this C++20
project, so it is most useful after a successful CMake configure has produced a
valid compile database.

Use Serena when you need:

- symbol definitions
- references
- type-aware navigation
- diagnostics
- safer edits across related C++ files

Health check:

```powershell
just serena-health
```

The full Serena logs are local under `.serena/logs/` and are ignored by git.
Run health checks from a normal terminal for the cleanest signal. Sandboxed
Codex tool runs can produce false Windows permission warnings when clangd or
Python tries to write outside the workspace.

If clangd starts crashing, check `.clang-tidy`, `.clangd`, the configured
compile database, and the latest `.serena/logs/health-checks/*.log` before
adding new language-server workarounds.

## Graphify

Graphify gives broad code graph context. It is useful before changing systems
that cross several files or before asking "what depends on this?"

Refresh the graph data manually:

```powershell
graphify update . --force --no-cluster
```

To regenerate the report and force-graph HTML for this project without using an
LLM label backend:

```powershell
$env:GRAPHIFY_VIZ_NODE_LIMIT = "10000"
graphify cluster-only . --graph graphify-out/graph.json --no-label
just graphify-name-communities
```

If community labels show up as `Community 0`, `Community 1`, and so on after any
Graphify run, use the local deterministic namer:

```powershell
just graphify-name-communities
```

Graphify also has an LLM-backed native label pass:

```powershell
graphify label .
```

That native pass needs a configured backend/API key. The `just` recipe does not;
it derives stable names from dominant source paths and symbols.

Useful queries:

```powershell
graphify query "How does player input reach puck shooting?" --graph graphify-out/graph.json
graphify explain "Hockey::Gameplay" --graph graphify-out/graph.json
graphify path "HockeyGameClient" "Hockey::Puck" --graph graphify-out/graph.json
graphify affected "Hockey::MatchSystem" --depth 2 --graph graphify-out/graph.json
```

Generated HTML lives under `graphify-out/` and stays ignored:

```text
graphify-out/graph.html
graphify-out/GRAPH_TREE.html
graphify-out/ZH2-callflow.html
```

Regenerate HTML when the graph is stale or when you want a visual map before a
large change. Do not commit generated Graphify output unless the project policy
explicitly changes.

## Beads

Beads is for persistent task and blocker tracking. It is not the source of truth
for architecture rules or phase status.

Use:

```powershell
bd ready --json
```

If Beads needs to be initialized again, keep it from taking over repo
instructions:

```powershell
bd init --skip-agents
```

## Superpowers

Superpowers is installed through the Codex plugin UI, not as a required repo
CLI. Use it for process guidance on unclear features, difficult bugs, gameplay
or physics design, networking design, replay/serialization design, and larger
architecture decisions.

Use the repo docs and source truth first. Superpowers should improve the
thinking process, not create another competing rule system.

## Dotagents And Skills

Use dotagents to validate the repo-level agent manifest:

```powershell
dotagents doctor
```

There are no project skills declared yet. Add skills only when there is a
stable, repeatable workflow that benefits from versioned instructions. Good
future candidates are phase closeout, gameplay replay triage, renderer visual
smoke triage, and asset import validation.

Do not copy all project rules into `.agents` skills. `AGENTS.md`,
`docs/ai-rules/`, `docs/phase-plans/`, and `docs/phase_status/` already cover
that role.

## What Not To Reintroduce

- Do not recreate `.cursor/` or Claude-specific project config unless explicitly
  requested.
- Do not add ccls fallback setup; the project AI flow uses clangd.
- Do not commit `.serena/cache/`, `.serena/logs/`, `graphify-out/`, build
  output, runtime logs, or AI diagnostic output.
- Do not use Graphify output as a replacement for source/CMake truth.
- Do not let Beads, dotagents, or skills duplicate the authoritative rules in
  `AGENTS.md`.

## Troubleshooting

If `just tools-check` reports a missing optional tool, install that tool or use
the lower-level project scripts until it is available. Superpowers is expected
to be reported as plugin UI setup, not a repo CLI.

If `just serena-health` fails, open the newest log under
`.serena/logs/health-checks/` and check the first real error. Permission errors
from sandboxed Codex runs are often false positives; rerun the same command in a
normal terminal before changing config.

If Graphify answers look stale, rerun:

```powershell
graphify update . --force --no-cluster
```

If verification fails, prefer fixing the underlying issue over weakening the AI
tooling. Report the exact failing command and log path when handing the task
back.
