# AI Docs

This folder is the entry point for AI-assisted work in ZH2.

Read `AGENTS.md` first. It is the authoritative rule file for architecture,
phase discipline, dependency boundaries, local-only state, and completion
requirements. The files here explain how to apply those rules efficiently.

## Read Path

For most coding tasks:

1. `AGENTS.md`
2. `docs/ai/onboarding.md`
3. The matching `docs/phase-status/phase-*.md`
4. The relevant source, headers, CMake target, config, and tests
5. A focused guide from this folder only when it matches the task

For debugging or visual verification, also read:

- `docs/ai/debugging.md`

For local AI tools and optional setup, read:

- `docs/ai/tooling.md`

## Source Of Truth

- Source, CMake, tests, runtime data, and `docs/phase-status/` define current
  implementation state and active phase context.
- `AGENTS.md` defines hard agent rules.
- `docs/project-structure.md` maps the current repository layout, target graph,
  scripts, app flags, and screenshot behavior.
- `docs/phase-rules/` contains compact phase constraints.
- `docs/phase-plans/` contains large detailed phase specs; load only the
  matching plan when detailed phase context is needed.
- `docs/ai-rules/` contains portable AI guidance retained from earlier rule
  systems. Use relevant files as supporting context.

If docs disagree with source/CMake truth, trust source/CMake first and update
the docs or call out the conflict.

## File Map

```text
docs/ai/
|-- README.md          This AI docs map
|-- onboarding.md      What to read before editing
|-- workflow.md        Day-to-day AI coding loop and final reporting
|-- tooling.md         Optional tool policy and local-only state
|-- debugging.md       Evidence, logs, screenshots, and failure triage
`-- hockey/            Focused hockey-domain guidance
```

## Focused Guides

Use the smallest relevant guide:

- `docs/ai/hockey/gameplay.md` for scoring, possession, shooting, passing,
  restarts, periods, and game rules.
- `docs/ai/hockey/physics.md` for puck movement, skating, friction, shots,
  stick contact, collisions, and deterministic simulation.
- `docs/ai/hockey/ai-behavior.md` for CPU skaters, goalies, positioning,
  tactics, steering, and difficulty tuning.
- `docs/ai/hockey/networking.md` for multiplayer, authoritative server work,
  input commands, snapshots, prediction, reconciliation, and finite tests.
- `docs/ai/hockey/arena-editor.md` for rink layout, boards, goals, spawns,
  arena serialization, validation, and editor/runtime agreement.
- `docs/ai/hockey/asset-pipeline.md` for hockey asset scale, orientation,
  naming, collision proxies, and import/export expectations.

Do not load every guide for small tasks.
