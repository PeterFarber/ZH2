# AI Rule Fragments

This folder contains portable `.mdc` rule fragments retained for tools that can
consume rule files.

`AGENTS.md` remains authoritative. `docs/ai/` explains the current AI workflow.
Use these fragments as supporting context, not as separate sources of truth.

## When To Read

- Read only the fragment that matches the task or tool integration.
- Prefer `AGENTS.md`, source/CMake truth, and `docs/phase-status/` when guidance
  conflicts.
- Keep fragments short and focused. Do not duplicate full phase plans here.

## Files

```text
000-project-overview.mdc              Project summary and phase discipline
010-architecture-boundaries.mdc       Dependency graph and forbidden links
020-cpp-cmake-style.mdc               C++ and CMake style rules
030-cross-platform-linux-windows.mdc  Windows/Linux compatibility rules
040-phase-discipline.mdc              Phase sequencing rules
050-phase-selection.mdc               Active phase selection rules
055-phase-status.mdc                  Phase status checklist rules
060-ai-workflow.mdc                   AI edit/verify loop
070-debugging-discipline.mdc          Evidence-first debugging
080-visual-verification.mdc           Screenshot and visual proof rules
```
