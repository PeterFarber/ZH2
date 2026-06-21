# Phase 1 - Foundation/Core

Status: Implemented, with display-backed runtime coverage still manual.

Source material:

- [x] Read `docs/phase-plans/phase-01-foundation-core.md`.
- [x] Read `docs/phase-rules/050-phase-1-foundation-core.mdc`.
- [x] Checked `docs/project-structure.md`.
- [x] Checked current source/CMake/test state.

## What This Phase Implements

- [x] Cross-platform C++20 project foundation.
- [x] `hockey_core` runtime library.
- [x] Thin app shells for client, editor, and dedicated server.
- [x] Core test runner.
- [x] Windows/Linux setup, configure, build, run, and test script foundation.
- [x] Runtime services required by later phases: logging, config, paths, timing, events, input, app loops, and jobs.

## Finished - Build And Project Skeleton

- [x] Root `CMakeLists.txt` wires the project and target order.
- [x] CMake presets exist for Windows debug/release and Linux debug/release.
- [x] vcpkg manifest mode is used for third-party dependencies.
- [x] `apps/game_client`, `apps/map_editor`, and `apps/dedicated_server` targets exist.
- [x] `apps/core_tests` exists for foundation-level tests.
- [x] Windows PowerShell scripts configure and build debug/release presets.
- [x] Linux Bash scripts configure and build debug/release presets.
- [x] Runtime apps accept `--root` so paths do not depend on current working directory.

## Finished - Core Runtime Services

- [x] `Status` and `Result<T>` style recoverable failure types exist.
- [x] Assertions are available for programmer errors.
- [x] spdlog-backed logging exists with core/client/editor/server/test loggers.
- [x] TOML config loading/saving exists.
- [x] Filesystem helpers use `std::filesystem`.
- [x] Project path resolution supports root-relative runtime paths.
- [x] Log, temp, data, and screenshot paths are centralized.
- [x] Platform helpers cover executable path, sleep, hardware thread count, and debugger checks.
- [x] Signal handler supports graceful Ctrl+C/SIGTERM style shutdown.
- [x] Crash/terminate handler logs fatal failures and is shut down on app teardown.
- [x] Command-line parser supports flag/value parsing and typed getters.

## Finished - Time, Events, Window, Input

- [x] Time helpers use `std::chrono::steady_clock`.
- [x] Timer utility exists.
- [x] Fixed timestep utility exists.
- [x] UUID generation and parsing exist.
- [x] Typed event queue exists.
- [x] SDL3 window wrapper exists.
- [x] SDL3 events are translated into engine event/input state.
- [x] Keyboard, mouse, gamepad, and text input plumbing exist.
- [x] Windowed app loop exists.
- [x] Headless app loop exists.
- [x] `app.sleep_when_idle` is honored by the headless app.
- [x] `input.gamepad_enabled` is applied at startup.

## Finished - Jobs And Screenshots

- [x] Thread pool exists.
- [x] Job system and `ParallelFor` style work distribution exist.
- [x] Screenshot path generation exists under `_save/screenshots/`.
- [x] Runtime logs write under `data/logs/` unless overridden by `--log`.

## Finished - Tests And Verification

- [x] Core tests cover UUID behavior.
- [x] Core tests cover config behavior.
- [x] Core tests cover path behavior.
- [x] Core tests cover filesystem behavior.
- [x] Core tests cover job system behavior.
- [x] Core tests cover fixed tick behavior.
- [x] Core tests cover command-line parsing.
- [x] Core tests cover event queue behavior.
- [x] Additional coverage exists for platform, time, timer, thread pool, signal handler, and paths.
- [x] Dedicated server can run headless.
- [x] Client and editor have windowed app entrypoints.
- [x] Windows and Linux debug test helpers run the full built test executable set.

## Started / Partial

- [ ] Window/SDL lifecycle behavior still needs display-backed smoke coverage.
- [ ] Keyboard, mouse, and gamepad behavior still need display/controller verification.
- [ ] Crash handler output still needs real crash/runtime verification.
- [ ] Logging output is covered at the library level but still benefits from runtime app verification.
- [ ] Windowed app loop behavior is not fully proven in headless-only environments.

## Left To Do

- [ ] Add display-backed smoke coverage when a GPU/display CI path exists.
- [ ] Add controller/device matrix testing when hardware is available.
- [ ] Keep `core` limited to platform/runtime basics only.
- [ ] Keep `core` free of ECS, renderer, physics, gameplay, editor, Vulkan, ImGui, GameNetworkingSockets, and future networking dependencies.
