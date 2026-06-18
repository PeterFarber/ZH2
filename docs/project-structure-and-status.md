# HockeyGame Project Structure and Status

This document describes the current source tree and implementation state of this
checkout. It is based on the CMake targets, public headers, source files, config
files, and tests present in the repository.

## Project Goal

HockeyGame is a custom C++20 hockey game and engine targeting a real-time 4v4
hockey experience:

- 2 teams
- 3 skaters and 1 goalie per team
- 8 player slots total
- 1 puck
- 2 goals
- Dedicated authoritative server simulation
- Client input, snapshots, prediction, interpolation, and reconciliation planned
  for the networking phase

The engine is split into small subsystem libraries and executable apps. Each
library owns a clear domain so renderer, editor, physics, gameplay, and future
networking code do not bleed across boundaries.

## Current Target Graph

The currently implemented CMake libraries are:

```text
hockey_core
hockey_ecs        -> hockey_core
hockey_assets     -> hockey_core
hockey_renderer   -> hockey_core, hockey_ecs
hockey_physics    -> hockey_core, hockey_ecs
hockey_gameplay   -> hockey_core, hockey_ecs, hockey_physics
hockey_editor     -> hockey_core, hockey_ecs, hockey_renderer,
                     hockey_assets, hockey_physics, hockey_gameplay
```

Important private dependencies:

- `hockey_assets` privately uses `fastgltf`, `meshoptimizer`, `shaderc`, `stb`,
  `yaml-cpp`, and `glm`.
- `hockey_renderer` privately owns Vulkan-facing dependencies: `volk`,
  `vulkan-headers`, `VulkanMemoryAllocator`, `shaderc`, and `stb_image_write`.
- `hockey_physics` privately owns Jolt. Jolt headers and symbols are not exposed
  to downstream targets.
- `hockey_editor` owns editor-only UI dependencies: Dear ImGui, ImGuizmo, native
  file dialogs, the vendored ImGui Vulkan backend, and Vulkan/volk bridge code.

There is no `hockey_networking` target yet. Networking, lobbies, replication,
prediction, interpolation, and reconciliation are still future work.

The main executable targets are:

```text
HockeyGameClient      -> core, ecs, renderer, assets, physics, gameplay
HockeyMapEditor       -> core, ecs, renderer, assets, editor, physics, gameplay
HockeyDedicatedServer -> core, ecs, physics, gameplay
HockeyAssetTool       -> core, assets
HockeyShaderTool      -> core, renderer
```

The dedicated server is intentionally headless. The root `CMakeLists.txt` fails
configuration if the server links renderer, editor, or ImGui targets.

## Repository Layout

```text
.
|-- apps/                    Executables, tools, and test runners
|-- data/                    Runtime config, raw assets, cooked assets, shaders
|-- docs/                    Project documentation and phase rule notes
|-- libs/                    Engine libraries
|-- scripts/                 Windows and Linux setup/build/run/test scripts
|-- CMakeLists.txt           Top-level target wiring and architectural guards
|-- CMakePresets.json        Configure/build presets
|-- vcpkg.json               Manifest-mode third-party dependencies
|-- AGENTS.md                Development rules and architecture constraints
|-- README.md                Main project overview
|-- LICENSE                  Project license
|-- .clang-format            Formatting rules
|-- .clang-tidy              Static analysis rules
|-- .editorconfig            Editor defaults
|-- project-words.txt        Project dictionary for spell checking
|-- icon.png                 App/project icon asset
```

### Layout Conventions

Most code follows this shape:

```text
libs/<library>/
|-- CMakeLists.txt
|-- include/Hockey/<Subsystem>/    Public headers
`-- src/                           Private implementation

apps/<app>/
|-- CMakeLists.txt
`-- src/                           Main, app class, commands, or tests
```

Important conventions:

- Public engine headers live under `include/Hockey/...` and are consumed through
  target include directories rather than relative include paths.
- Library `.cpp` files live under `src/` and own private implementation detail.
- Each library owns its own `CMakeLists.txt`; the root `CMakeLists.txt` only
  wires target order and architecture guards.
- Test runners are regular apps under `apps/*_tests`, one executable per
  subsystem.
- Runtime data is rooted at `data/`; application command-line `--root` points
  path resolution at the repository root.
- Raw asset metadata is stored beside raw assets as `.meta.yaml` files.
- Cooked assets are produced under `data/cooked/` by the editor or asset tool.

### `apps/`

```text
apps/
|-- game_client/        Playable windowed client (`HockeyGameClient`)
|-- map_editor/         Unity-style editor (`HockeyMapEditor`)
|-- dedicated_server/   Headless authoritative simulation app
|-- asset_tool/         Asset pipeline command-line tool
|-- shader_tool/        Shader compilation command-line tool
|-- core_tests/         Core runtime test executable
|-- ecs_tests/          ECS/scene/prefab test executable
|-- asset_tests/        Asset pipeline test executable
|-- renderer_tests/     Renderer/settings/shader/render-graph tests
|-- editor_tests/       Editor workflow/tool/preview tests
|-- physics_tests/      Physics world/component/query tests
`-- gameplay_tests/     Hockey gameplay simulation tests
```

App source files are intentionally thin. The apps compose subsystem libraries,
load config, register components, open scenes, and run the relevant app loop.
Most reusable behavior belongs in `libs/`.

### `libs/`

```text
libs/
|-- core/       Platform/runtime foundation
|-- ecs/        Entity, component, scene, prefab, and validation data
|-- assets/     CPU-side asset import, cook, database, and runtime loading
|-- renderer/   Vulkan renderer, render resources, frame path, screenshots
|-- physics/    Jolt-backed physics facade and ECS physics sync
|-- gameplay/   Hockey rules, input, match, puck, player, and snapshot systems
`-- editor/     Dear ImGui editor, panels, tools, gizmos, previews
```

Subsystem notes:

- `libs/core` is the only place that should own broad runtime services such as
  paths, config, logging, input, application loops, jobs, and timing.
- `libs/ecs` is data-oriented and renderer/physics/gameplay agnostic except for
  plain component data and metadata.
- `libs/assets` owns file and content transformation. It does not upload GPU
  resources directly.
- `libs/renderer` owns Vulkan implementation files under `src/Vulkan/` and
  exposes renderer handles and high-level renderer APIs through public headers.
- `libs/physics` exposes a Jolt-free public facade. Jolt-facing files live under
  `src/Jolt/`; Jolt headers are treated as internal implementation detail.
- `libs/gameplay` is split by hockey domain: `Match`, `Player`, `Puck`, `Rink`,
  `Stick`, `Teams`, `Tuning`, `Validation`, and `Simulation`.
- `libs/editor` is split by editor domain: `Panels`, `Tools`, `Gizmos`,
  `Inspector`, `Project`, `ImGui`, and `Logging`. Its vendored ImGui Vulkan
  backend lives under `libs/editor/vendor/imgui_backend/`.

### `data/`

```text
data/
|-- config/       App configs: client, editor, server, editor layout
|-- gameplay/     Default gameplay rules and tuning YAML
|-- raw/          Source assets and authoring files
|-- cooked/       Cooked asset database and cooked asset payloads
|-- editor/       User/editor settings and layout state
|-- shaders/      GLSL sources and compiled SPIR-V binaries
|-- logs/         Runtime log output
`-- temp/         Temporary files used by tests/editor/runtime flows
```

Raw asset structure:

```text
data/raw/
|-- scenes/       Authoring scenes and renderer/physics/gameplay test scenes
|-- prefabs/      Authored reusable entity hierarchies
|-- materials/    Material YAML assets and metadata
|-- models/       GLB models and extracted texture folders
`-- textures/     Standalone texture assets and metadata
```

Shader structure:

```text
data/shaders/
|-- src/          GLSL source files
`-- bin/          Compiled SPIR-V files consumed by the renderer
```

### `docs/`

```text
docs/
|-- project-structure-and-status.md
|-- ai-debugging-playbook.md
|-- phase_status/
|   |-- README.md
|   |-- phase-01-foundation-core.md
|   |-- phase-02-ecs-scene-prefab.md
|   |-- phase-03-vulkan-renderer.md
|   |-- phase-04-unity-style-editor.md
|   |-- phase-05-asset-pipeline.md
|   |-- phase-06-physics.md
|   |-- phase-07-hockey-gameplay.md
|   |-- phase-08-networking-lobbies.md
|   `-- phase-09-polish-animation-audio-ui.md
`-- phase-rules/
    |-- 050-phase-1-foundation-core.mdc
    |-- 060-phase-2-ecs-scene-prefab.mdc
    |-- 070-phase-3-vulkan-renderer.mdc
    |-- 080-phase-4-unity-style-editor.mdc
    |-- 090-phase-5-asset-pipeline.mdc
    |-- 100-phase-6-physics.mdc
    |-- 110-phase-7-hockey-gameplay.mdc
    |-- 120-phase-8-networking-lobbies.mdc
    `-- 130-phase-9-polish-animation-audio-ui.mdc
```

### `scripts/`

```text
scripts/
|-- windows/
|   |-- setup.ps1
|   |-- configure_debug.ps1
|   |-- configure_release.ps1
|   |-- build_debug.ps1
|   |-- build_release.ps1
|   |-- build_shaders.ps1
|   |-- run_client.ps1
|   |-- run_editor.ps1
|   |-- run_server.ps1
|   |-- test.ps1
|   |-- Env.ps1
|   |-- Prerequisites.ps1
|   `-- README.md
`-- linux/
    |-- setup.sh
    |-- configure_debug.sh
    |-- configure_release.sh
    |-- build_debug.sh
    |-- build_release.sh
    |-- run_client.sh
    |-- run_editor.sh
    |-- run_server.sh
    `-- test.sh
```

The scripts are thin wrappers over CMake presets and built executables. Windows
scripts use PowerShell; Linux scripts use Bash.

### Build and Dependency Files

```text
CMakeLists.txt       Adds libraries/apps/tests and enforces dependency guards
CMakePresets.json    Ninja presets for windows-debug, windows-release,
                     linux-debug, and linux-release
vcpkg.json           Manifest dependencies for SDL3, Vulkan, ImGui, Jolt,
                     asset import, shader compilation, and utility libraries
```

### Generated or Local-Output Directories

```text
build/                 CMake/Ninja build outputs and compile_commands mirrors
out/                   Local AI diagnostics and other ignored generated output
vcpkg_installed/       vcpkg installed packages
.vcpkg/                vcpkg local state
_save/                 User/runtime saves, screenshots, and similar local output
data/logs/             Runtime logs
data/temp/             Temporary runtime/editor files
```

## Building, Running, and Screenshotting

### Prerequisites

The project uses:

- C++20
- CMake presets
- Ninja
- vcpkg manifest mode
- SDL3
- Vulkan headers/runtime support
- PowerShell scripts on Windows
- Bash scripts on Linux

Windows setup expects Visual Studio 2022 with the C++ toolchain. The Windows
scripts load the Visual Studio developer environment, use the Visual Studio CMake
and Ninja binaries when available, set `VCPKG_ROOT` to the repo-local `.vcpkg/`
checkout, and bootstrap vcpkg when needed.

Linux setup expects a C++20 compiler, CMake, Ninja, Git, pkg-config style build
tools, archive utilities, and SDL/Vulkan window-system development packages. The
Linux setup script can install common dependencies for apt, pacman, and dnf
systems. `VCPKG_ROOT` should point at a vcpkg checkout; the setup script will
bootstrap it if the checkout exists but is not bootstrapped.

### First-Time Windows Flow

Run these from the repository root:

```powershell
.\scripts\windows\setup.ps1
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

What this does:

- `setup.ps1` checks/install tools, loads shared environment helpers, creates or
  bootstraps `.vcpkg/`, and verifies CMake/Ninja availability.
- `configure_debug.ps1` removes stale CMake cache state when the vcpkg toolchain
  path changed, then runs `cmake --preset windows-debug`.
- `build_debug.ps1` runs `cmake --build --preset build-windows-debug`.
- `test.ps1` runs all debug test executables that are part of the current test
  suite.

Release build:

```powershell
.\scripts\windows\configure_release.ps1
.\scripts\windows\build_release.ps1
```

Equivalent direct CMake commands:

```powershell
cmake --preset windows-debug
cmake --build --preset build-windows-debug
cmake --preset windows-release
cmake --build --preset build-windows-release
```

### First-Time Linux Flow

Set up vcpkg first if `VCPKG_ROOT` is not already configured:

```bash
git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/vcpkg"
```

Then run from the repository root:

```bash
./scripts/linux/setup.sh
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
```

Release build:

```bash
./scripts/linux/configure_release.sh
./scripts/linux/build_release.sh
```

Equivalent direct CMake commands:

```bash
cmake --preset linux-debug
cmake --build --preset build-linux-debug
cmake --preset linux-release
cmake --build --preset build-linux-release
```

Current Linux note: `scripts/linux/test.sh` only runs `HockeyCoreTests`. To run
the rest of the test executables, call them directly from `build/linux-debug/`
or expand the script.

### Build Outputs

Debug outputs:

```text
build/windows-debug/apps/game_client/HockeyGameClient.exe
build/windows-debug/apps/map_editor/HockeyMapEditor.exe
build/windows-debug/apps/dedicated_server/HockeyDedicatedServer.exe
build/windows-debug/apps/asset_tool/HockeyAssetTool.exe
build/windows-debug/apps/shader_tool/HockeyShaderTool.exe

build/linux-debug/apps/game_client/HockeyGameClient
build/linux-debug/apps/map_editor/HockeyMapEditor
build/linux-debug/apps/dedicated_server/HockeyDedicatedServer
build/linux-debug/apps/asset_tool/HockeyAssetTool
build/linux-debug/apps/shader_tool/HockeyShaderTool
```

Release outputs use `build/windows-release/` or `build/linux-release/` with the
same app subdirectories.

### Running Apps

Windows wrappers pass any extra arguments through to the executable and inject
the repository root with `--root`:

```powershell
.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1
```

Examples:

```powershell
.\scripts\windows\run_client.ps1 --max-frames 5
.\scripts\windows\run_editor.ps1 --scene data/raw/scenes/main_rink.scene.yaml
.\scripts\windows\run_server.ps1 --max-ticks 300
```

Current Linux wrappers run the default apps with `--root .` and do not forward
extra arguments:

```bash
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```

Use direct executable commands when passing flags on Linux:

```bash
./build/linux-debug/apps/game_client/HockeyGameClient --root . --max-frames 5
./build/linux-debug/apps/map_editor/HockeyMapEditor --root . --scene data/raw/scenes/main_rink.scene.yaml
./build/linux-debug/apps/dedicated_server/HockeyDedicatedServer --root . --max-ticks 300
```

### Common App Flags

Game client:

```text
--root <dir>             Project root directory
--config <file>          Override client config TOML path
--log <file>             Override client log file path
--fps-limit <n>          Override frame-rate cap
--max-frames <n>         Auto-quit after n rendered frames
--screenshot [prefix]    Screenshot from the first rendered frame
--screenshot-prefix <p>  Prefix for automated screenshot filenames
--vk-validation          Enable Vulkan validation layers
--physics-debug          Enable physics debug drawing
--help, -h               Print usage
```

Map editor:

```text
--root <dir>             Project root directory
--config <file>          Override editor config TOML path
--log <file>             Override editor log file path
--scene <file>           Open a scene on startup
--max-frames <n>         Auto-quit after n rendered frames
--frames <n>             Alias for --max-frames
--capture-viewports      Save both Scene and Game viewport screenshots
--capture-scene-view     Save only the Scene viewport screenshot
--capture-game-view      Save only the Game viewport screenshot
--capture-prefix <p>     Prefix for automated viewport screenshots
--capture-width <n>      Automated viewport screenshot width
--capture-height <n>     Automated viewport screenshot height
--screenshot             Save a full editor-window screenshot
--screenshot-window      Code-supported alias for full-window screenshot
--screenshot-prefix <p>  Prefix for automated full-window screenshot filenames
--screenshot-frame <n>   Frame to capture full editor window, default 3
--vk-validation          Enable Vulkan validation layers
--help, -h               Print usage
```

Dedicated server:

```text
--root <dir>             Project root directory
--config <file>          Override server config TOML path
--log <file>             Override server log file path
--max-ticks <n>          Run bounded simulation and exit after n ticks
--help, -h               Print usage
```

### Testing

Windows all-tests wrapper:

```powershell
.\scripts\windows\test.ps1
```

The Windows test wrapper currently runs:

```text
HockeyCoreTests
HockeyECSTests
HockeyAssetTests
HockeyRendererTests
HockeyEditorTests
HockeyPhysicsTests
HockeyGameplayTests
```

Linux current wrapper:

```bash
./scripts/linux/test.sh
```

Direct Linux test commands:

```bash
./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/asset_tests/HockeyAssetTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./build/linux-debug/apps/editor_tests/HockeyEditorTests --root .
./build/linux-debug/apps/physics_tests/HockeyPhysicsTests --root .
./build/linux-debug/apps/gameplay_tests/HockeyGameplayTests --root .
```

Direct Windows test commands follow the same app paths under
`build/windows-debug/`, with `.exe` suffixes.

### Asset Tool

The asset tool is not wrapped by a top-level script yet. Call it directly:

```powershell
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . discover
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . import-all
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . cook-all-dirty
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . recook-all
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . validate
```

Linux:

```bash
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . discover
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . import-all
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . cook-all-dirty
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . recook-all
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . validate
```

Asset tool commands:

```text
discover          Scan data/raw, create/update metadata, save database
import <raw-path> Import one raw file
import-all        Import all supported raw files
cook <asset-id>   Cook one asset
cook-all-dirty    Cook dirty assets and dependents
recook-all        Force-recook all supported assets
validate          Check files, cooked outputs, and dependencies
```

### Shader Tool

Windows shader wrapper:

```powershell
.\scripts\windows\build_shaders.ps1
```

That script builds `HockeyShaderTool` in the debug tree and runs it with the
repository root. Direct commands:

```powershell
.\build\windows-debug\apps\shader_tool\HockeyShaderTool.exe --root .
.\build\windows-debug\apps\shader_tool\HockeyShaderTool.exe --root . --src data/shaders/src --bin data/shaders/bin
```

Linux:

```bash
cmake --build --preset build-linux-debug --target HockeyShaderTool
./build/linux-debug/apps/shader_tool/HockeyShaderTool --root .
./build/linux-debug/apps/shader_tool/HockeyShaderTool --root . --src data/shaders/src --bin data/shaders/bin
```

### Script Reference

Windows scripts:

```text
setup.ps1              Install/check prerequisites, configure VS env, bootstrap vcpkg
configure_debug.ps1    Configure `windows-debug`
configure_release.ps1  Configure `windows-release`
build_debug.ps1        Build `build-windows-debug`
build_release.ps1      Build `build-windows-release`
build_shaders.ps1      Build and run `HockeyShaderTool`
ai_smoke.ps1           Collect AI diagnostics under `out/ai_diagnostics/`
run_client.ps1         Run debug `HockeyGameClient` with `--root <repo>` and pass-through args
run_editor.ps1         Run debug `HockeyMapEditor` with `--root <repo>` and pass-through args
run_server.ps1         Run debug `HockeyDedicatedServer` with `--root <repo>` and pass-through args
test.ps1               Run all debug subsystem test executables
Env.ps1                Shared repo-root, VS, CMake, Ninja, and vcpkg environment helper
Prerequisites.ps1      Windows prerequisite detection/installation helpers
```

Linux scripts:

```text
setup.sh               Install common Linux build deps and bootstrap existing vcpkg checkout
configure_debug.sh     Configure `linux-debug`
configure_release.sh   Configure `linux-release`
build_debug.sh         Build `build-linux-debug`
build_release.sh       Build `build-linux-release`
ai_smoke.sh            Collect AI diagnostics under `out/ai_diagnostics/`
run_client.sh          Run debug `HockeyGameClient` with `--root .`
run_editor.sh          Run debug `HockeyMapEditor` with `--root .`
run_server.sh          Run debug `HockeyDedicatedServer` with `--root .`
test.sh                Run debug `HockeyCoreTests` with `--root .`
```

### AI Diagnostics

AI debugging and screenshot evidence should be collected under:

```text
out/ai_diagnostics/
```

`_save/` is reserved for user/runtime saves and engine screenshot output. The AI
smoke scripts trigger built-in screenshots with unique `ai_*` prefixes and move
only those captures into the diagnostics bundle.

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

See `docs/ai-debugging-playbook.md` for triage flows and reporting guidance.

### Screenshot Output

All built-in screenshots use `Hockey::MakeScreenshotPath()` and write PNG files
under:

```text
_save/screenshots/
```

The filename format is:

```text
<sanitized-prefix>_YYYYMMDD_HHMMSS.png
```

Prefix rules:

- Alphanumeric characters are lowercased.
- `-` and `_` are preserved.
- Other characters collapse to `_`.
- A trailing `_` is removed.
- An empty prefix becomes `screenshot`.
- If a file already exists for that second, `_1`, `_2`, and so on are appended.

### Screenshot Modes

Game client:

- Press F12 while running to queue a gameplay screenshot with the `game` prefix.
- Use `--screenshot [prefix]` to capture the first rendered frame.
- Use `--screenshot-prefix <prefix>` to set the automated prefix.

Map editor full-window capture:

- Use `--screenshot` to capture the full editor window.
- Use `--screenshot-prefix <prefix>` to control the filename prefix.
- Use `--screenshot-frame <n>` to wait for a later rendered frame. The default
  is frame 3 so the editor has time to initialize.

Map editor viewport capture:

- Click the `Capture` button over the Scene or Game viewport.
- Press F12 while the Scene or Game viewport is hovered.
- Use `--capture-viewports` to capture both Scene and Game viewports.
- Use `--capture-scene-view` or `--capture-game-view` for one viewport.
- Use `--capture-prefix <prefix>` to name automated viewport screenshots.
- Use `--capture-width <n>` and `--capture-height <n>` for automated viewport
  capture resolution. Defaults are 1920 by 1080.

Automated viewport filenames append the viewport role:

```text
<prefix>_scene_YYYYMMDD_HHMMSS.png
<prefix>_game_YYYYMMDD_HHMMSS.png
```

Manual viewport captures use these prefixes:

```text
editor_scene
editor_game
```

### Screenshot Examples

Windows game client first-frame capture:

```powershell
.\scripts\windows\run_client.ps1 --screenshot gameplay --max-frames 5
```

Windows full editor-window capture:

```powershell
.\scripts\windows\run_editor.ps1 --screenshot --screenshot-prefix editor_window --screenshot-frame 3 --max-frames 5
```

Windows editor viewport captures:

```powershell
.\scripts\windows\run_editor.ps1 --capture-viewports --capture-prefix rink --capture-width 1920 --capture-height 1080 --max-frames 5
```

Linux direct game client capture:

```bash
./build/linux-debug/apps/game_client/HockeyGameClient --root . --screenshot gameplay --max-frames 5
```

Linux direct editor viewport captures:

```bash
./build/linux-debug/apps/map_editor/HockeyMapEditor --root . --capture-viewports --capture-prefix rink --capture-width 1920 --capture-height 1080 --max-frames 5
```

### Screenshot Implementation Notes

- Screenshot requests are queued; the renderer records a GPU image-to-buffer copy
  during rendering and writes the PNG after the frame fence confirms the GPU is
  done with that frame.
- Full-window screenshots copy from the swapchain image and require swapchain
  `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` support. If the surface does not support
  transfer-source swapchain images, full-window screenshots fail cleanly.
- Editor viewport screenshots copy from renderer-owned offscreen render targets.
- PNG writing supports 8-bit RGBA/BGRA render targets. BGRA is converted to RGBA
  before writing.
- Bounded screenshot runs should allow enough frames for capture and readback.
  Use at least a few frames; for editor full-window capture, keep
  `--max-frames` greater than or equal to `--screenshot-frame`.

## Apps

### `apps/game_client`

Builds `HockeyGameClient`, the playable windowed client.

Implemented:

- SDL3 window/input through `hockey_core`.
- Vulkan renderer initialization and scene rendering.
- Renderer settings and startup scene loading from `data/config/client.toml`.
- Physics system initialization and fixed-step update.
- Local gameplay simulation using `GameplayWorld`.
- Local input mapping for movement, aiming, boost, brake, quick turn, shooting,
  and poke check.
- Reset match hotkey.
- Optional gameplay event logging.
- Optional physics debug draw.
- Built-in screenshot support through F12 and `--screenshot`.
- Bounded `--max-frames` runs for smoke testing.

Current limitation:

- This is still local play. It does not connect to an authoritative network
  server, and it does not run prediction/reconciliation yet.

### `apps/map_editor`

Builds `HockeyMapEditor`, the Unity-style editor application.

Implemented:

- Docked ImGui editor shell.
- Hierarchy, Inspector, Scene View, Game View, Project, Console, Stats,
  Properties, Scene Validation, Prefab, Physics, and Preferences panels.
- Scene file loading/saving and workflow commands.
- Selection, undo/redo, clipboard, and entity subtree snapshots.
- Transform, light, and hockey-specific authoring tools.
- Scene and game viewport rendering through renderer-owned offscreen targets.
- Viewport screenshot buttons and automated viewport capture flags.
- Asset discovery, import, cooking, dirty tracking, hot reload hooks, and project
  browser integration.
- Physics preview with collider/trigger visualization.
- Gameplay preview that snapshots the authored scene, runs fixed gameplay ticks,
  accepts local preview input, and restores the authored scene on stop/reset.

Current limitation:

- Much of the editor UI needs real display/GPU verification. The non-UI logic is
  covered by editor tests, but panel rendering and interactive viewport behavior
  remain hard to fully validate headlessly.

### `apps/dedicated_server`

Builds `HockeyDedicatedServer`, the headless simulation app.

Implemented:

- Headless app lifecycle.
- Startup scene loading from `data/config/server.toml`.
- Scene validation on load.
- Fixed tick loop.
- Physics system integration.
- Authoritative local gameplay simulation through `GameplayWorld`.
- Gameplay event logging.
- Bounded `--max-ticks` runs for verification.

Current limitation:

- No socket transport or remote clients yet. The server runs authoritative scene,
  physics, and gameplay logic, but networking is not implemented.

### `apps/asset_tool`

Builds `HockeyAssetTool`.

Implemented:

- Command-line access to the asset pipeline.
- Asset discovery/import/cook workflows backed by `hockey_assets`.

### `apps/shader_tool`

Builds `HockeyShaderTool`.

Implemented:

- Command-line shader compilation support through renderer shader tooling.

## Engine Libraries

### `libs/core`

Foundation/runtime library.

Implemented:

- Logging.
- Platform helpers and executable path handling.
- Signal and crash handling.
- Command-line parsing.
- Project-root-relative paths and temp/log file paths.
- Config loading/saving with TOML.
- Time, timer, and fixed-timestep utilities.
- UUIDs.
- Event queue.
- SDL3-backed window, keyboard, mouse, gamepad, and input state.
- Thread pool and job system.
- Windowed and headless application base classes.
- Screenshot path generation under `_save/screenshots`.

Boundary:

- `core` should remain platform/runtime basics only. It must not depend on ECS,
  renderer, physics, gameplay, editor, Vulkan, ImGui, or future networking.

### `libs/ecs`

Entity, component, scene, and prefab data library.

Implemented:

- EnTT-backed entity wrapper.
- Scene ownership and lifecycle.
- Systems and scene modes.
- Transform and hierarchy components.
- Core authoring/game components.
- Render-facing data components.
- Component registry and metadata for editor inspection.
- Component serialization helpers.
- YAML scene serialization/deserialization.
- Scene validation.
- Scene events.
- Prefabs, prefab serialization, prefab overrides, apply/revert support, and
  instance diffing.

Boundary:

- `ecs` stores data and scene structure. It should not know about Vulkan, ImGui,
  SDL windowing, renderer internals, physics internals, or gameplay rules.

### `libs/assets`

CPU-side content pipeline.

Implemented:

- Asset IDs, paths, hashes, handles, events, metadata, and database.
- Asset manager and in-memory registry.
- Raw-to-cooked import/cook framework.
- File watcher and hot reload event plumbing.
- Importers for textures, materials, glTF models, shaders, scenes, and prefabs.
- Cookers for textures, materials, meshes, models, shaders, scenes, and prefabs.
- Runtime CPU-side loaders for textures, meshes, models, materials, shaders, and
  prefabs.
- Material YAML serialization.
- Scene dependency scanning.
- Cooked asset database serialization.

Boundary:

- `assets` owns CPU content only. Renderer/editor consume it; it must not link
  renderer, editor, ECS, physics, gameplay, or future networking.

### `libs/renderer`

Vulkan renderer library.

Implemented:

- Vulkan instance, surface, device, allocator, swapchain, command, frame, buffer,
  texture, sampler, shader, descriptor, mesh, and pipeline wrappers.
- Renderer pimpl API with opaque handles exposed to consumers.
- Built-in resource and material setup.
- Asset-backed mesh, material, and texture upload/caching.
- Forward PBR scene rendering.
- Directional shadow cascades.
- Spot/point local shadow atlas.
- Depth prepass and SSAO.
- Bloom downsample/upsample/composite.
- Tonemapping and FXAA output path.
- Debug line and debug shape rendering.
- Offscreen render targets for editor Scene/Game viewports.
- Renderer settings load/save from config.
- Material preview support for editor UI.
- Screenshot readback to PNG for swapchain and render targets.
- A `RenderGraph` abstraction and tests.

Current limitations:

- The production frame sequence is still hard-coded in `Renderer.cpp`; the
  render graph exists but is not the main execution path.
- FXAA is implemented. TAA/MSAA settings exist but fall back to the current
  output path.
- Reflection probes and decals serialize in ECS, but renderer systems for them
  are not implemented.
- Hockey-specific visual polish such as ice reflections, skate spray, and puck
  trails is not implemented as renderer systems yet.

### `libs/physics`

Jolt-backed physics library.

Implemented:

- Public physics facade that hides Jolt from downstream libraries.
- Physics settings load/save.
- Collision layers and layer filtering.
- Physics materials and combine behavior.
- Rigid body component.
- Box, sphere, capsule, cylinder, and mesh collider components.
- Trigger component and trigger filtering.
- Physics component YAML serialization and editor metadata.
- Collider validation and shape build validation.
- Scene-to-physics sync.
- Fixed-step `PhysicsSystem`.
- Contact and trigger event queues.
- Ray casts and shape casts.
- Debug draw collection for colliders, triggers, contact points, and physics
  overlays.
- Mesh collider support through an injected `PhysicsMeshProvider`, keeping
  physics independent from assets.

Boundary:

- `physics` owns simulation only. It must not link renderer, editor, gameplay,
  networking, Vulkan, or ImGui.

### `libs/gameplay`

Hockey rules and simulation library.

Implemented:

- Gameplay settings from config and YAML tuning/rules data.
- Team, role, and player-slot types for 3 skaters plus 1 goalie per team.
- Gameplay input frame and input buffering.
- Gameplay event queue and event names.
- Gameplay component serialization and editor metadata.
- Match state, score state, team state, player, skater, goalie, puck, possession,
  stick, goal, out-of-play, and faceoff components.
- Match initialization and scene validation for gameplay-ready scenes.
- Match clock, faceoff flow, reset flow, and score system.
- Player movement.
- Puck controller and puck possession.
- Stick handling, shooting, passing, and checking.
- Goal detection and out-of-play handling.
- `GameplayWorld` fixed-step orchestration over scene, physics, input, events,
  and tuning.
- Gameplay snapshot generation for match, players, and puck state.
- Main rink gameplay validation and regression coverage.
- Client local play integration.
- Editor gameplay preview integration.
- Headless server gameplay integration.

Current limitations:

- Gameplay is local/authoritative inside the process that runs it. It is not yet
  replicated over a network transport.
- `SceneMode::ClientPrediction` exists for future prediction, but prediction and
  reconciliation are not implemented.

### `libs/editor`

Unity-style editor library.

Implemented:

- Editor app, context, settings, camera, dockspace, menu bar, toolbar, and panel
  manager.
- ImGui layer, renderer bridge, and editor theme.
- Scene workflow and file dialogs.
- Selection model.
- Undo/redo command stack.
- Clipboard and entity/component snapshot commands.
- Inspector field drawers, add-component menu, material picker, and component
  inspector.
- Project browser, file type registry, asset drag/drop, prefab drag/drop, and
  asset previews.
- Hierarchy, Inspector, Scene Viewport, Game Viewport, Project, Console, Stats,
  Scene Validation, Prefab, Physics, Preferences, and Properties panels.
- Grid, selection outline, physics, and transform gizmos.
- Transform, light, and hockey-specific authoring tools.
- Physics preview and gameplay preview.
- Editor console sink.

Boundary:

- `editor` may inspect gameplay data and run preview worlds, but core gameplay
  simulation lives in `libs/gameplay`.

## Data Layout

```text
data/config/       App config TOML files
data/gameplay/     Default gameplay rules and tuning YAML
data/raw/          Source assets authored by the project
data/cooked/       Cooked assets and asset database
data/editor/       Editor layout/settings/theme-adjacent data
data/shaders/src/  GLSL shader sources
data/shaders/bin/  Compiled SPIR-V shader binaries
data/logs/         Local logs
data/temp/         Local temp files
```

Important raw asset groups:

- `data/raw/scenes/` contains render tests, physics smoke scenes, and the
  gameplay rink scene.
- `data/raw/prefabs/` contains authored player/goalie prefabs.
- `data/raw/materials/`, `data/raw/models/`, and `data/raw/textures/` contain
  source content and metadata.

Important shader sources:

- `mesh.vert`
- `pbr.frag`
- `depth_only.vert`
- `shadow.vert` / `shadow.frag`
- `ssao.frag` / `ssao_blur.frag`
- `bloom_downsample.frag` / `bloom_upsample.frag`
- `tonemap.frag`
- `fxaa.frag`
- `debug_line.*`
- `debug_shape.*`
- `fullscreen_triangle.vert`
- `common.glsl`

## Tests

When `HOCKEY_BUILD_TESTS` is enabled, the repository builds subsystem test apps:

```text
HockeyCoreTests
HockeyECSTests
HockeyAssetTests
HockeyRendererTests
HockeyEditorTests
HockeyPhysicsTests
HockeyGameplayTests
```

Coverage currently includes:

- Core UUID, config, paths, filesystem, jobs, fixed ticks, command line, events,
  platform, thread pool, and time behavior.
- ECS entity, component, hierarchy, transform, scene serialization, validation,
  prefab, metadata, lifecycle, scene events, and render component behavior.
- Asset IDs, metadata, paths, database, texture import, glTF import, materials,
  shader cooking, dependencies, and hot reload behavior.
- Renderer settings, resource handles, shader compilation, render graph behavior,
  renderer smoke paths, and server link invariants.
- Editor settings, selection, inspector fields, undo/redo, tools, hockey physics
  setup, gameplay preview, scene workflow, and project browser behavior.
- Physics initialization, collision layers, materials, components, colliders,
  world stepping, rigid bodies, scene sync, contacts, triggers, queries, debug
  draw, and headless server physics.
- Gameplay settings/tuning, input, events, teams, components, match setup,
  gameplay world ticking, reset, player movement, puck interaction/controller,
  shooting, passing, checking, goals, out of play, snapshots, main rink setup,
  and headless server gameplay.

Windows helper:

```powershell
.\scripts\windows\test.ps1
```

Linux helper:

```bash
./scripts/linux/test.sh
```

Current note: the Linux helper only runs `HockeyCoreTests`; use the direct
commands in the building section to run the other Linux test executables.

## Scripts

Windows scripts:

```text
scripts/windows/setup.ps1
scripts/windows/configure_debug.ps1
scripts/windows/configure_release.ps1
scripts/windows/build_debug.ps1
scripts/windows/build_release.ps1
scripts/windows/build_shaders.ps1
scripts/windows/ai_smoke.ps1
scripts/windows/run_client.ps1
scripts/windows/run_editor.ps1
scripts/windows/run_server.ps1
scripts/windows/test.ps1
```

Linux scripts:

```text
scripts/linux/setup.sh
scripts/linux/configure_debug.sh
scripts/linux/configure_release.sh
scripts/linux/build_debug.sh
scripts/linux/build_release.sh
scripts/linux/ai_smoke.sh
scripts/linux/run_client.sh
scripts/linux/run_editor.sh
scripts/linux/run_server.sh
scripts/linux/test.sh
```

## Phase Status Snapshot

```text
Phase 1 - Foundation/core:        Implemented
Phase 2 - ECS/scene/prefab:       Implemented
Phase 3 - Vulkan renderer:        Implemented, with known renderer gaps
Phase 4 - Unity-style editor:     Implemented, with UI/GPU verification gaps
Phase 5 - Asset pipeline:         Implemented, with advanced format gaps
Phase 6 - Physics:                Implemented
Phase 7 - Hockey gameplay:        Implemented for local/client/editor/server use
Phase 8 - Networking/lobbies:     Not implemented
Phase 9 - Animation/audio/UI/polish: Not implemented
```

Per-phase checkbox status files live in:

```text
docs/phase_status/
```

Read `docs/phase_status/README.md` before using the per-phase files. It defines
how AI agents should interpret checked items, partial items, not-started items,
and future-phase work.

## Major Not-Yet-Implemented Areas

- `libs/networking` and GameNetworkingSockets integration.
- Client/server protocol, input replication, snapshots over the wire, and lobby
  flow.
- Client prediction, interpolation, and reconciliation.
- Animation systems.
- Audio systems.
- Final game UI/HUD/menus.
- Visual polish systems such as skate spray, puck trails, richer ice rendering,
  reflection probes, and decals.
- Production render graph migration.
- TAA/MSAA implementation.

## Architectural Guardrails

The repository enforces several boundaries in CMake and should continue to do so:

- Dedicated server remains headless.
- Game client does not link editor UI.
- Assets remain CPU-side and do not link renderer/editor/ECS/physics/gameplay.
- Physics remains independent from renderer/editor/gameplay/UI.
- Gameplay remains independent from renderer/editor/networking/platform UI.
- Renderer owns Vulkan details behind opaque public APIs.
- Jolt stays private to `hockey_physics`.
- Editor can preview physics/gameplay but does not own those simulation systems.
