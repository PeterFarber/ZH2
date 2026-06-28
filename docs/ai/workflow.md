# AI Workflow

Use this loop for day-to-day AI-assisted work in ZH2.

## Core Loop

1. Read the task and identify the owning subsystem.
2. Orient with `AGENTS.md`, `docs/ai/onboarding.md`, phase status, and nearby
   source/CMake/tests.
3. Use `rg` for exact text and file discovery.
4. Use Serena for symbol lookup, references, diagnostics, and semantic edits.
5. Use Graphify before broad architecture questions or high-blast-radius
   refactors, then verify its conclusions against source.
6. Use Beads only for persistent tasks, blockers, dependencies, or follow-up
   work that should survive the session.
7. Keep the change scoped to the owning boundary.
8. Run the narrowest meaningful verification first.
9. Run `just verify` when the change warrants the full completion contract.
10. Follow `docs/phase-status/README.md` before finalizing.

## Before Editing

- Stay on the current git branch. Do not create worktrees, create new branches,
  or switch branches unless the user explicitly asks for that git operation.
- Read the matching `docs/phase-status/phase-*.md`.
- Determine the active phase from the user's request, owning subsystem, and
  matching phase status file.
- For phase-owned work, read the matching `docs/phase-rules/phase-*.mdc`.
- Read `docs/phase-plans/phase-*.md` only when detailed phase context is needed.
- Inspect current source, headers, CMake, config, and tests.
- Check dependency direction before adding includes, links, or new targets.
- Prefer existing patterns over new abstractions.
- For debugging, read `docs/ai/debugging.md` and reproduce first.
- For hockey-domain work, read only the matching `docs/ai/hockey/` guide.

## While Editing

- Keep diffs focused.
- Avoid unrelated refactors.
- Do not modify local AI state, generated graph output, logs, caches, build
  output, or cooked assets unless explicitly required.
- Update CMake when adding or removing source files.
- Add or update focused tests for behavior changes.
- Preserve Windows/Linux compatibility.

## Verification

Use the narrowest useful check first. Examples:

```powershell
just build-debug
just test
just smoke-text
just verify
```

For renderer, editor UI, shader, camera, viewport, or screenshot changes, follow
`docs/ai/debugging.md` for visual verification.

For docs-only changes, no build is required unless the docs update changes
scripts, commands, generated files, or build configuration.

## Final Response

Report:

- What changed.
- Focused verification that ran.
- Any verification that could not run and why.
- Any risks or follow-up work.
- Phase-status outcome, following `docs/phase-status/README.md`.
