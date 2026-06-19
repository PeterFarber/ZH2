# Phase 1 Plan — Complete Cross-Platform Foundation/Core

File path recommendation:

```text
docs/phase-plans/phase-01-foundation-core.md
```

AI instruction:

```text
Read `docs/phase-plans/phase-01-foundation-core.md` and implement Phase 1 exactly. Do not implement ECS, renderer, networking, physics, gameplay, editor UI, ImGui, Vulkan, or asset pipeline code during this phase. Work step by step, update CMake whenever files are added, and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 1 builds the complete reusable foundation for the hockey game project.

This phase creates the base project, build system, cross-platform runtime layer, application framework, SDL3 window/input layer, headless server loop, logging/config/filesystem utilities, job system, and core tests.

Phase 1 must support:

```text
Windows game client
Linux game client
Windows map editor
Linux map editor
Windows dedicated server
Linux dedicated server
```

Phase 1 must produce:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
HockeyCoreTests
hockey_core
```

Phase 1 must not contain:

```text
Vulkan
renderer
ECS
scene system
prefabs
physics
networking
GameNetworkingSockets
hockey gameplay
map editor UI
ImGui
asset pipeline
audio
animation
```

---

# 1. Target Project Structure

Create or verify this project structure:

```text
HockeyGame/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
  README.md
  AGENTS.md
  .gitignore
  .editorconfig
  .clang-format
  .clang-tidy

  docs/ai-rules/
    rules/
      000-project-overview.mdc
      010-architecture-boundaries.mdc
      020-cpp-cmake-style.mdc
      030-cross-platform-linux-windows.mdc
      040-phase-discipline.mdc
      050-current-phase.mdc
    plans/
      phase-01-foundation-core.md

  apps/
    game_client/
      CMakeLists.txt
      src/
        GameClientMain.cpp
        GameClientApp.hpp
        GameClientApp.cpp

    map_editor/
      CMakeLists.txt
      src/
        MapEditorMain.cpp
        MapEditorApp.hpp
        MapEditorApp.cpp

    dedicated_server/
      CMakeLists.txt
      src/
        DedicatedServerMain.cpp
        DedicatedServerApp.hpp
        DedicatedServerApp.cpp

    core_tests/
      CMakeLists.txt
      src/
        CoreTestsMain.cpp
        Test.hpp
        UUIDTests.cpp
        ConfigTests.cpp
        PathTests.cpp
        FileSystemTests.cpp
        JobSystemTests.cpp
        FixedTickTests.cpp
        CommandLineTests.cpp
        EventQueueTests.cpp

  libs/
    core/
      CMakeLists.txt
      include/Hockey/Core/
        Base.hpp
        Types.hpp
        Result.hpp
        Assert.hpp

        Log.hpp
        Platform.hpp
        SignalHandler.hpp
        CrashHandler.hpp

        CommandLine.hpp
        FileSystem.hpp
        Paths.hpp
        Config.hpp

        Time.hpp
        Timer.hpp
        FixedTimestep.hpp
        UUID.hpp

        Event.hpp
        EventQueue.hpp

        Window.hpp
        Input.hpp
        Keyboard.hpp
        Mouse.hpp
        Gamepad.hpp

        Application.hpp
        WindowedApplication.hpp
        HeadlessApplication.hpp

        ThreadPool.hpp
        JobSystem.hpp

      src/
        Log.cpp
        Platform.cpp
        SignalHandler.cpp
        CrashHandler.cpp

        CommandLine.cpp
        FileSystem.cpp
        Paths.cpp
        Config.cpp

        Time.cpp
        Timer.cpp
        FixedTimestep.cpp
        UUID.cpp

        EventQueue.cpp

        Window.cpp
        Input.cpp
        Keyboard.cpp
        Mouse.cpp
        Gamepad.cpp

        Application.cpp
        WindowedApplication.cpp
        HeadlessApplication.cpp

        ThreadPool.cpp
        JobSystem.cpp

  data/
    config/
      client.toml
      editor.toml
      server.toml
    logs/
    raw/
    cooked/
    temp/

  scripts/
    windows/
      setup.ps1
      configure_debug.ps1
      configure_release.ps1
      build_debug.ps1
      build_release.ps1
      run_client.ps1
      run_editor.ps1
      run_server.ps1
      test.ps1

    linux/
      setup.sh
      configure_debug.sh
      configure_release.sh
      build_debug.sh
      build_release.sh
      run_client.sh
      run_editor.sh
      run_server.sh
      test.sh
```

---

# 2. Dependencies

Use C++20, CMake, Ninja, and vcpkg manifest mode.

`vcpkg.json`:

```json
{
  "name": "hockey-game",
  "version": "0.1.0",
  "dependencies": [
    "sdl3",
    "spdlog",
    "fmt",
    "glm",
    "tomlplusplus"
  ]
}
```

Dependency purpose:

```text
SDL3          window creation, events, keyboard, mouse, gamepad
spdlog        console/file logging
fmt           string formatting
glm           math types used by current/future systems
tomlplusplus  TOML config load/save
```

Linux setup packages for Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config \
  zip \
  unzip \
  tar \
  curl \
  libx11-dev \
  libxext-dev \
  libxrandr-dev \
  libxcursor-dev \
  libxi-dev \
  libxinerama-dev \
  libwayland-dev \
  libxkbcommon-dev \
  wayland-protocols
```

---

# 3. CMake Requirements

## 3.1 Root `CMakeLists.txt`

Implement:

```cmake
cmake_minimum_required(VERSION 3.25)

project(HockeyGame
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HOCKEY_BUILD_TESTS "Build Hockey engine tests" ON)
option(HOCKEY_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_subdirectory(libs/core)

add_subdirectory(apps/game_client)
add_subdirectory(apps/map_editor)
add_subdirectory(apps/dedicated_server)

if(HOCKEY_BUILD_TESTS)
    add_subdirectory(apps/core_tests)
endif()
```

## 3.2 CMake Presets

`CMakePresets.json` must include:

```text
windows-debug
windows-release
linux-debug
linux-release

build-windows-debug
build-windows-release
build-linux-debug
build-linux-release
```

Use Ninja.

Use the vcpkg toolchain file:

```text
$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
```

for Windows presets and the same environment variable for Linux presets.

## 3.3 Core Target

`libs/core/CMakeLists.txt` must create:

```text
hockey_core
```

Use imported targets:

```cmake
SDL3::SDL3
spdlog::spdlog
fmt::fmt
glm::glm
tomlplusplus::tomlplusplus
```

Do not use global include directories.

Do not use global link directories.

Do not add renderer/ECS/networking/physics dependencies.

## 3.4 App Targets

Create these executables:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
HockeyCoreTests
```

All should link `hockey_core`.

Do not make the server link any future window/render/editor dependencies.

---

# 4. Platform Compatibility Rules

The project must work on Windows and Linux.

## 4.1 Forbidden

Do not hardcode:

```text
.exe
data\config\client.toml
Windows-only APIs in random files
Linux-only APIs in random files
current working directory assumptions
```

Do not spread platform ifdefs through normal engine/game/editor/server code.

## 4.2 Required

Use:

```text
std::filesystem::path
path / "child"
Paths::ConfigFile(...)
Paths::LogFile(...)
Paths::DataFile(...)
Platform::ExecutablePath()
Platform::SleepMilliseconds(...)
Platform::YieldThread()
SignalHandler::Install()
SignalHandler::ShutdownRequested()
```

## 4.3 Platform-Specific Files

Only these files may contain platform-specific OS logic:

```text
Platform.cpp
SignalHandler.cpp
CrashHandler.cpp
possibly FileSystem.cpp
```

---

# 5. Core Library Modules

## 5.1 `Types.hpp`

Implement fixed-width aliases inside namespace `Hockey`:

```cpp
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;
```

## 5.2 `Base.hpp`

Implement common base macros:

```cpp
#define HK_UNUSED(x) (void)(x)
```

Add other non-invasive base utilities only if needed.

## 5.3 `Result.hpp`

Implement:

```cpp
struct Status {
    bool success = false;
    std::string error;

    static Status Ok();
    static Status Fail(std::string message);

    explicit operator bool() const;
};

template<typename T>
struct Result {
    bool success = false;
    T value{};
    std::string error;

    static Result<T> Ok(T value);
    static Result<T> Fail(std::string message);

    explicit operator bool() const;
};
```

Use `Status` / `Result<T>` for recoverable engine failures:

```text
missing config
bad TOML
file read/write failure
window creation failure
bad command-line option
```

Do not use exceptions for normal engine errors.

## 5.4 `Assert.hpp`

Implement debug assertions:

```cpp
#if HK_CONFIG_DEBUG
    #define HK_ASSERT(condition, message, ...) ...
#else
    #define HK_ASSERT(condition, message, ...)
#endif
```

Assertions are for programmer errors only.

Do not assert on user/config/file/network errors.

---

# 6. Logging System

## 6.1 Files

```text
Log.hpp
Log.cpp
```

## 6.2 Requirements

Logging must support:

```text
console logging
file logging
timestamps
thread IDs
severity levels
separate named loggers
flush on warning/error
safe shutdown
```

## 6.3 Required Loggers

Implement:

```cpp
class Log {
public:
    static bool Init(const std::filesystem::path& logFile);
    static void Shutdown();

    static std::shared_ptr<spdlog::logger>& Core();
    static std::shared_ptr<spdlog::logger>& Client();
    static std::shared_ptr<spdlog::logger>& Editor();
    static std::shared_ptr<spdlog::logger>& Server();
    static std::shared_ptr<spdlog::logger>& Tests();
};
```

## 6.4 Required Macros

Implement:

```cpp
HK_CORE_TRACE(...)
HK_CORE_INFO(...)
HK_CORE_WARN(...)
HK_CORE_ERROR(...)
HK_CORE_CRITICAL(...)

HK_CLIENT_INFO(...)
HK_EDITOR_INFO(...)
HK_SERVER_INFO(...)
HK_TEST_INFO(...)
```

## 6.5 Required Log Files

```text
data/logs/client.log
data/logs/editor.log
data/logs/server.log
data/logs/core_tests.log
```

## 6.6 Done Criteria

Logging is complete when:

```text
log folder is auto-created
client writes client.log
editor writes editor.log
server writes server.log
tests write core_tests.log
console output works
logs flush before shutdown
```

---

# 7. Platform System

## 7.1 Files

```text
Platform.hpp
Platform.cpp
```

## 7.2 API

Implement:

```cpp
class Platform {
public:
    static std::filesystem::path ExecutablePath();
    static std::filesystem::path CurrentWorkingDirectory();

    static std::string OSName();

    static uint32_t HardwareThreadCount();

    static void SleepMilliseconds(uint32_t ms);
    static void SleepMicroseconds(uint64_t us);
    static void YieldThread();

    static bool IsDebuggerAttached();
};
```

## 7.3 Windows Implementation Notes

Use only in `Platform.cpp`:

```text
GetModuleFileNameW
GetCurrentDirectoryW
Sleep
IsDebuggerPresent
```

## 7.4 Linux Implementation Notes

Use only in `Platform.cpp`:

```text
/proc/self/exe
getcwd
std::this_thread::sleep_for
std::this_thread::yield
```

## 7.5 Done Criteria

Platform is complete when:

```text
ExecutablePath works on Windows
ExecutablePath works on Linux
OSName reports platform
Sleep functions work
HardwareThreadCount works
No platform APIs leak into app code
```

---

# 8. Signal Handler

## 8.1 Files

```text
SignalHandler.hpp
SignalHandler.cpp
```

## 8.2 API

Implement:

```cpp
class SignalHandler {
public:
    static void Install();
    static bool ShutdownRequested();
    static void Reset();

private:
    static std::atomic_bool s_ShutdownRequested;
};
```

## 8.3 Windows Behavior

Handle:

```text
CTRL_C_EVENT
CTRL_CLOSE_EVENT
CTRL_BREAK_EVENT
```

Use `SetConsoleCtrlHandler` only in `SignalHandler.cpp`.

## 8.4 Linux Behavior

Handle:

```text
SIGINT
SIGTERM
```

Use `sigaction` only in `SignalHandler.cpp`.

## 8.5 Done Criteria

Signal handling is complete when:

```text
Ctrl+C stops server on Windows
Ctrl+C stops server on Linux
SIGTERM stops server on Linux
server exits cleanly
logs are flushed
```

---

# 9. Crash Handler

## 9.1 Files

```text
CrashHandler.hpp
CrashHandler.cpp
```

## 9.2 API

Implement:

```cpp
class CrashHandler {
public:
    static void Install();
    static void Shutdown();
};
```

## 9.3 Behavior

Phase 1 must:

```text
install std::terminate handler
log fatal termination message
flush logs before exit
```

Do not add platform minidump complexity yet unless simple and isolated.

---

# 10. Command Line Parser

## 10.1 Files

```text
CommandLine.hpp
CommandLine.cpp
```

## 10.2 Supported Options

```text
--root <path>
--config <path>
--log <path>
--headless
--tick-rate <number>
--port <number>
--fps-limit <number>
--help
```

## 10.3 API

```cpp
class CommandLine {
public:
    CommandLine() = default;
    CommandLine(int argc, char** argv);

    bool Has(std::string_view flag) const;

    std::string GetString(std::string_view flag, std::string defaultValue = "") const;
    int GetInt(std::string_view flag, int defaultValue = 0) const;
    double GetDouble(std::string_view flag, double defaultValue = 0.0) const;
    bool GetBool(std::string_view flag, bool defaultValue = false) const;

    const std::vector<std::string>& Args() const;
};
```

## 10.4 Parsing Rules

Support:

```text
--key value
--flag
```

Examples:

```bash
./HockeyDedicatedServer --root . --tick-rate 120 --port 27020
```

```powershell
.\HockeyDedicatedServer.exe --root . --tick-rate 120 --port 27020
```

---

# 11. Filesystem System

## 11.1 Files

```text
FileSystem.hpp
FileSystem.cpp
```

## 11.2 API

```cpp
class FileSystem {
public:
    static bool Exists(const std::filesystem::path& path);
    static bool IsFile(const std::filesystem::path& path);
    static bool IsDirectory(const std::filesystem::path& path);

    static Status CreateDirectories(const std::filesystem::path& path);
    static Status Remove(const std::filesystem::path& path);

    static Result<std::string> ReadTextFile(const std::filesystem::path& path);
    static Status WriteTextFile(const std::filesystem::path& path, std::string_view text);

    static std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& dir);
    static std::filesystem::path Normalize(const std::filesystem::path& path);
};
```

## 11.3 Rules

Use `std::filesystem`.

Do not concatenate path strings manually.

Create parent directories before writing files.

Return `Status` / `Result<T>` for failures.

---

# 12. Paths System

## 12.1 Files

```text
Paths.hpp
Paths.cpp
```

## 12.2 API

```cpp
struct EnginePaths {
    std::filesystem::path root;
    std::filesystem::path data;
    std::filesystem::path config;
    std::filesystem::path logs;
    std::filesystem::path rawAssets;
    std::filesystem::path cookedAssets;
    std::filesystem::path temp;
};

class Paths {
public:
    static bool Init(const std::filesystem::path& executablePath,
                     const std::filesystem::path& rootOverride = {});

    static const EnginePaths& Get();

    static std::filesystem::path ConfigFile(const std::string& filename);
    static std::filesystem::path LogFile(const std::string& filename);
    static std::filesystem::path DataFile(const std::string& relative);
    static std::filesystem::path RawAsset(const std::string& relative);
    static std::filesystem::path CookedAsset(const std::string& relative);
    static std::filesystem::path TempFile(const std::string& filename);
};
```

## 12.3 Root Resolution

Support command-line root override:

```text
--root <path>
```

If root override is provided, use it.

Otherwise infer project root from executable path as best as possible.

## 12.4 Required Directories

Ensure these exist:

```text
data/logs
data/temp
```

Do not fail if `data/raw` and `data/cooked` do not yet have content.

---

# 13. Config System

## 13.1 Files

```text
Config.hpp
Config.cpp
```

## 13.2 API

```cpp
class Config {
public:
    Status Load(const std::filesystem::path& path);
    Status Save(const std::filesystem::path& path) const;

    bool Has(std::string_view key) const;

    std::string GetString(std::string_view key, std::string defaultValue = "") const;
    int GetInt(std::string_view key, int defaultValue = 0) const;
    double GetDouble(std::string_view key, double defaultValue = 0.0) const;
    bool GetBool(std::string_view key, bool defaultValue = false) const;

    void SetString(std::string_view key, std::string value);
    void SetInt(std::string_view key, int value);
    void SetDouble(std::string_view key, double value);
    void SetBool(std::string_view key, bool value);
};
```

## 13.3 Key Style

Use dot notation:

```text
app.name
app.target_fps
window.width
window.height
server.tick_rate
server.port
```

## 13.4 Required Config Files

`data/config/client.toml`:

```toml
[app]
name = "Hockey Game Client"
target_fps = 0

[window]
title = "Hockey Game"
width = 1920
height = 1080
resizable = true
maximized = false
start_centered = true

[input]
gamepad_enabled = true
mouse_capture = false
```

`data/config/editor.toml`:

```toml
[app]
name = "Hockey Map Editor"
target_fps = 0

[window]
title = "Hockey Map Editor"
width = 1600
height = 900
resizable = true
maximized = true
start_centered = true

[input]
gamepad_enabled = true
mouse_capture = false
```

`data/config/server.toml`:

```toml
[app]
sleep_when_idle = true

[server]
name = "Local Hockey Server"
tick_rate = 60
max_players = 8
port = 27020
```

---

# 14. Time and Timer System

## 14.1 Files

```text
Time.hpp
Time.cpp
Timer.hpp
Timer.cpp
```

## 14.2 `Time` API

```cpp
class Time {
public:
    static double NowSeconds();
    static uint64_t NowNanoseconds();
};
```

Use `std::chrono::steady_clock`.

Do not use system clock for frame/tick timing.

## 14.3 `Timer` API

```cpp
class Timer {
public:
    Timer();

    void Reset();

    double ElapsedSeconds() const;
    double ElapsedMilliseconds() const;
};
```

---

# 15. Fixed Timestep

## 15.1 Files

```text
FixedTimestep.hpp
FixedTimestep.cpp
```

## 15.2 API

```cpp
class FixedTimestep {
public:
    explicit FixedTimestep(double tickRate = 60.0);

    void SetTickRate(double tickRate);
    double GetTickRate() const;

    double GetFixedDeltaSeconds() const;
    uint64_t GetTick() const;

    int Accumulate(double deltaSeconds);
    void AdvanceTick();
    void Reset();

private:
    double m_TickRate = 60.0;
    double m_FixedDelta = 1.0 / 60.0;
    double m_Accumulator = 0.0;
    uint64_t m_Tick = 0;
};
```

## 15.3 Behavior

The dedicated server uses fixed tick.

Default:

```text
60 Hz
```

Support:

```text
server.toml tick_rate
--tick-rate override
```

Never simulate the dedicated server with variable delta.

---

# 16. UUID System

## 16.1 Files

```text
UUID.hpp
UUID.cpp
```

## 16.2 API

```cpp
class UUID {
public:
    UUID();
    explicit UUID(uint64_t value);

    uint64_t Value() const;
    std::string ToString() const;

    bool IsValid() const;

    bool operator==(const UUID& other) const = default;
    bool operator!=(const UUID& other) const = default;

private:
    uint64_t m_Value = 0;
};
```

Add `std::hash<Hockey::UUID>`.

## 16.3 Requirements

```text
generates nonzero random IDs
usable as unordered_map key
serializable as string later
stable value when constructed from uint64_t
```

---

# 17. Event System

## 17.1 Files

```text
Event.hpp
EventQueue.hpp
EventQueue.cpp
```

## 17.2 Event Types

Implement:

```cpp
enum class EventType {
    None,

    WindowClose,
    WindowResize,
    WindowMinimized,
    WindowRestored,
    WindowFocusGained,
    WindowFocusLost,

    KeyPressed,
    KeyReleased,
    TextInput,

    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,

    GamepadConnected,
    GamepadDisconnected,
    GamepadButtonPressed,
    GamepadButtonReleased,
    GamepadAxisMoved
};
```

## 17.3 Event Struct

Use a simple debuggable struct:

```cpp
struct Event {
    EventType type = EventType::None;

    uint32_t windowWidth = 0;
    uint32_t windowHeight = 0;

    int key = 0;
    bool keyRepeat = false;

    int mouseButton = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;

    int gamepadId = -1;
    int gamepadButton = 0;
    int gamepadAxis = 0;
    float gamepadAxisValue = 0.0f;
};
```

## 17.4 EventQueue API

```cpp
class EventQueue {
public:
    void Push(const Event& event);
    bool Poll(Event& outEvent);
    void Clear();
    bool Empty() const;
};
```

---

# 18. Window System

## 18.1 Files

```text
Window.hpp
Window.cpp
```

## 18.2 API

```cpp
struct WindowDesc {
    std::string title = "Hockey App";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool resizable = true;
    bool maximized = false;
    bool startCentered = true;
};

class Window {
public:
    Window() = default;
    ~Window();

    bool Create(const WindowDesc& desc);
    void Destroy();

    void PollEvents(EventQueue& events);

    bool ShouldClose() const;
    void RequestClose();

    uint32_t Width() const;
    uint32_t Height() const;

    bool IsMinimized() const;
    bool IsFocused() const;

    SDL_Window* SDLHandle() const;

private:
    SDL_Window* m_Window = nullptr;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;

    bool m_ShouldClose = false;
    bool m_Minimized = false;
    bool m_Focused = true;
};
```

## 18.3 Requirements

```text
Create SDL3 window
Destroy SDL3 window
Handle close
Handle resize
Handle minimize
Handle restore
Handle focus gained/lost
Convert SDL window events into engine events
Work on Windows
Work on Linux
```

Do not implement Vulkan surface creation in Phase 1.

---

# 19. Input System

## 19.1 Files

```text
Input.hpp
Input.cpp
Keyboard.hpp
Keyboard.cpp
Mouse.hpp
Mouse.cpp
Gamepad.hpp
Gamepad.cpp
```

## 19.2 Requirements

Input system must support:

```text
keyboard current/down state
keyboard pressed/released this frame
mouse position
mouse delta
mouse buttons
mouse scroll
gamepad connected/disconnected
gamepad buttons
gamepad axes
gamepad rumble API
```

## 19.3 Input API

```cpp
class Input {
public:
    static void Init();
    static void Shutdown();

    static void BeginFrame();
    static void ProcessEvent(const Event& event);
    static void EndFrame();

    static std::vector<Gamepad> ConnectedGamepads();
};
```

## 19.4 Hockey-Focused Gamepad Coverage

Because this is a hockey game, Phase 1 must detect/read:

```text
left stick
right stick
left trigger
right trigger
face buttons
shoulders
start/back
stick buttons
```

Do not implement gameplay bindings yet.

---

# 20. Application System

## 20.1 Files

```text
Application.hpp
Application.cpp
WindowedApplication.hpp
WindowedApplication.cpp
HeadlessApplication.hpp
HeadlessApplication.cpp
```

## 20.2 Base Application API

```cpp
class Application {
public:
    explicit Application(CommandLine commandLine);
    virtual ~Application() = default;

    int Run();

    void RequestQuit();
    bool IsRunning() const;

protected:
    virtual bool OnInit() = 0;
    virtual void OnShutdown() = 0;

    CommandLine& GetCommandLine();

private:
    CommandLine m_CommandLine;
    bool m_Running = true;
};
```

## 20.3 Windowed Application

Used by:

```text
HockeyGameClient
HockeyMapEditor
```

Must:

```text
initialize SDL3
create window
poll events
update input
call OnEvent
call OnUpdate(deltaTime)
support FPS cap
shutdown cleanly
```

API:

```cpp
class WindowedApplication : public Application {
public:
    using Application::Application;

protected:
    bool CreateAppWindowFromConfig(const Config& config);
    Window& GetWindow();

    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnEvent(const Event& event) {}

private:
    int RunWindowed();

    Window m_Window;
    EventQueue m_EventQueue;
};
```

## 20.4 Headless Application

Used by:

```text
HockeyDedicatedServer
```

Must:

```text
not create a window
not require display server
install signal handler
run fixed tick
check shutdown signal
sleep/yield when idle
log tick overruns
shutdown cleanly
```

API:

```cpp
class HeadlessApplication : public Application {
public:
    using Application::Application;

protected:
    void SetTickRate(double tickRate);

    virtual void OnFixedUpdate(double fixedDeltaSeconds) = 0;

private:
    int RunHeadless();

    FixedTimestep m_Timestep{60.0};
};
```

---

# 21. ThreadPool and JobSystem

## 21.1 Files

```text
ThreadPool.hpp
ThreadPool.cpp
JobSystem.hpp
JobSystem.cpp
```

## 21.2 ThreadPool API

```cpp
class ThreadPool {
public:
    ThreadPool() = default;
    ~ThreadPool();

    void Start(uint32_t workerCount);
    void Stop();

    void Submit(std::function<void()> job);
    void WaitIdle();

    uint32_t WorkerCount() const;
};
```

## 21.3 JobSystem API

```cpp
class JobSystem {
public:
    static void Init(uint32_t workerCount = 0);
    static void Shutdown();

    static void Submit(std::function<void()> job);
    static void WaitIdle();

    static void ParallelFor(size_t count, std::function<void(size_t)> fn);

    static uint32_t WorkerCount();
};
```

## 21.4 Requirements

```text
workerCount 0 uses max(1, hardware_threads - 1)
jobs execute
exceptions in jobs are caught/logged
WaitIdle waits
ParallelFor visits every index exactly once
Shutdown joins all threads
works on Windows/Linux
```

---

# 22. App Implementations

## 22.1 Game Client

Files:

```text
GameClientMain.cpp
GameClientApp.hpp
GameClientApp.cpp
```

Behavior:

```text
construct CommandLine
initialize paths from --root
initialize logging to client.log
install crash handler
initialize job system
load client.toml
create SDL3 window
log platform info
log connected gamepads
run variable timestep
close on Escape/window close
shutdown job system
shutdown logging
```

## 22.2 Map Editor

Files:

```text
MapEditorMain.cpp
MapEditorApp.hpp
MapEditorApp.cpp
```

Behavior:

```text
construct CommandLine
initialize paths from --root
initialize logging to editor.log
install crash handler
initialize job system
load editor.toml
create SDL3 window
log platform info
run variable timestep
close on window close
shutdown job system
shutdown logging
```

## 22.3 Dedicated Server

Files:

```text
DedicatedServerMain.cpp
DedicatedServerApp.hpp
DedicatedServerApp.cpp
```

Behavior:

```text
construct CommandLine
initialize paths from --root
initialize logging to server.log
install signal handler
install crash handler
initialize job system
load server.toml
read tick_rate
allow --tick-rate override
run fixed timestep
log tick count periodically
log tick overruns
exit on Ctrl+C/SIGTERM
shutdown job system
shutdown logging
```

---

# 23. Scripts

## 23.1 Windows

Create:

```text
scripts/windows/setup.ps1
scripts/windows/configure_debug.ps1
scripts/windows/configure_release.ps1
scripts/windows/build_debug.ps1
scripts/windows/build_release.ps1
scripts/windows/run_client.ps1
scripts/windows/run_editor.ps1
scripts/windows/run_server.ps1
scripts/windows/test.ps1
```

Required content examples:

```powershell
cmake --preset windows-debug
cmake --build --preset build-windows-debug
.\build\windows-debug\apps\game_client\HockeyGameClient.exe --root .
```

## 23.2 Linux

Create:

```text
scripts/linux/setup.sh
scripts/linux/configure_debug.sh
scripts/linux/configure_release.sh
scripts/linux/build_debug.sh
scripts/linux/build_release.sh
scripts/linux/run_client.sh
scripts/linux/run_editor.sh
scripts/linux/run_server.sh
scripts/linux/test.sh
```

Required content examples:

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake --preset linux-debug
cmake --build --preset build-linux-debug
./build/linux-debug/apps/game_client/HockeyGameClient --root .
```

Make scripts executable:

```bash
chmod +x scripts/linux/*.sh
```

---

# 24. Core Tests

## 24.1 Test Target

Create:

```text
HockeyCoreTests
```

## 24.2 Simple Test Harness

Implement a basic test macro system in `apps/core_tests/src/Test.hpp`.

Requirements:

```text
run test functions
count passed/failed assertions
print readable failures
return nonzero if any test fails
```

## 24.3 Required Tests

### UUIDTests

```text
generated UUID is nonzero
generated UUIDs differ
UUID can be unordered_map key
UUID ToString is not empty
```

### ConfigTests

```text
loads client.toml
reads window.width
reads window.height
missing value returns default
saves temp config
reloads temp config
bad config returns failure
```

### PathTests

```text
root path resolves
config path resolves
log path resolves
temp path resolves
required directories are created
paths are normalized/absolute
```

### FileSystemTests

```text
write text file
read text file
create directory
list files
missing file returns failure
```

### JobSystemTests

```text
init workers
submit 1000 jobs
all jobs execute
ParallelFor covers every index once
WaitIdle waits correctly
shutdown cleanly
```

### FixedTickTests

```text
60 Hz tick has 1/60 delta
accumulating 1 second produces 60 ticks
tick counter advances
reset clears accumulator and tick
```

### CommandLineTests

```text
parses --root
parses --tick-rate
parses --port
missing values return defaults
```

### EventQueueTests

```text
push event
poll event
empty returns false
clear empties queue
```

---

# 25. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Build Skeleton

```text
1. Verify/create directory structure
2. Add vcpkg.json
3. Add root CMakeLists.txt
4. Add CMakePresets.json
5. Add libs/core/CMakeLists.txt
6. Add app CMake files
7. Add empty app entry points that compile
8. Confirm CMake configure/build
```

## Step 2 — Core Basics

```text
1. Add Types.hpp
2. Add Base.hpp
3. Add Result.hpp
4. Add Log.hpp/cpp
5. Add Assert.hpp
6. Add Platform.hpp/cpp
7. Add SignalHandler.hpp/cpp
8. Add CrashHandler.hpp/cpp
```

## Step 3 — Files, Paths, Config

```text
1. Add CommandLine.hpp/cpp
2. Add FileSystem.hpp/cpp
3. Add Paths.hpp/cpp
4. Add Config.hpp/cpp
5. Add data/config/client.toml
6. Add data/config/editor.toml
7. Add data/config/server.toml
8. Add tests for command-line/filesystem/paths/config
```

## Step 4 — Time and IDs

```text
1. Add Time.hpp/cpp
2. Add Timer.hpp/cpp
3. Add FixedTimestep.hpp/cpp
4. Add UUID.hpp/cpp
5. Add tests for UUID and fixed timestep
```

## Step 5 — Events, Window, Input

```text
1. Add Event.hpp
2. Add EventQueue.hpp/cpp
3. Add Window.hpp/cpp
4. Add Input.hpp/cpp
5. Add Keyboard.hpp/cpp
6. Add Mouse.hpp/cpp
7. Add Gamepad.hpp/cpp
8. Add event queue tests
```

## Step 6 — Application Loops

```text
1. Add Application.hpp/cpp
2. Add WindowedApplication.hpp/cpp
3. Add HeadlessApplication.hpp/cpp
4. Wire windowed loop
5. Wire headless fixed tick loop
```

## Step 7 — Jobs

```text
1. Add ThreadPool.hpp/cpp
2. Add JobSystem.hpp/cpp
3. Add job system tests
```

## Step 8 — Apps

```text
1. Implement GameClientApp
2. Implement MapEditorApp
3. Implement DedicatedServerApp
4. Confirm client opens window
5. Confirm editor opens window
6. Confirm server runs headless
```

## Step 9 — Scripts

```text
1. Add Windows scripts
2. Add Linux scripts
3. Confirm scripts call the correct build outputs
```

## Step 10 — Final Verification

```text
1. Build Debug Windows
2. Build Release Windows
3. Build Debug Linux
4. Build Release Linux
5. Run HockeyCoreTests
6. Run client
7. Run editor
8. Run server
9. Confirm logs/configs/paths
10. Confirm no future-phase code exists
```

---

# 26. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./scripts/linux/test.sh
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```

---

# 27. Phase 1 Definition of Done

Phase 1 is complete only when all of this is true:

```text
clean clone configures on Windows
clean clone configures on Linux
Debug builds on Windows
Release builds on Windows
Debug builds on Linux
Release builds on Linux
vcpkg manifest resolves dependencies
HockeyGameClient builds
HockeyMapEditor builds
HockeyDedicatedServer builds
HockeyCoreTests builds
client opens SDL3 window on Windows
client opens SDL3 window on Linux
editor opens SDL3 window on Windows
editor opens SDL3 window on Linux
server runs headless on Windows
server runs headless on Linux
Ctrl+C stops server on Windows
Ctrl+C/SIGTERM stops server on Linux
config loads on all apps
config saves in tests
logs write to console
logs write to files
paths resolve correctly
keyboard input works
mouse input works
gamepad detection works
job system works
fixed timestep works
core tests pass
no renderer code exists
no ECS code exists
no physics code exists
no networking code exists
no gameplay code exists
```

---

# 28. AI Completion Instruction

When Phase 1 is complete, AI agent should report:

```text
Phase 1 complete.

Implemented:
- cross-platform CMake/vcpkg project
- hockey_core
- game client
- map editor
- dedicated server
- core tests
- logging/config/paths/filesystem
- command line/platform/signal/crash handling
- time/timer/fixed timestep/UUID
- SDL3 window/event/input/gamepad
- app loops
- job system
- Windows/Linux scripts

Verified:
- build commands
- tests
- client window
- editor window
- headless server
- logs/configs
- no future-phase systems
```
