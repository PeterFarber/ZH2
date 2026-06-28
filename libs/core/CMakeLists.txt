find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

add_library(hockey_core STATIC)

target_sources(hockey_core
    PRIVATE
        src/Log.cpp
        src/Platform.cpp
        src/SignalHandler.cpp
        src/CrashHandler.cpp
        src/CommandLine.cpp
        src/FileSystem.cpp
        src/Paths.cpp
        src/Screenshot.cpp
        src/Config.cpp
        src/RuntimeConfig.cpp
        src/RuntimeConfigDefaults.cpp
        src/ResourceProvider.cpp
        src/Time.cpp
        src/Timer.cpp
        src/FixedTimestep.cpp
        src/UUID.cpp
        src/EventQueue.cpp
        src/Window.cpp
        src/Input.cpp
        src/Keyboard.cpp
        src/Mouse.cpp
        src/Gamepad.cpp
        src/ThreadPool.cpp
        src/JobSystem.cpp
        src/Application.cpp
        src/WindowedApplication.cpp
        src/HeadlessApplication.cpp
)

target_include_directories(hockey_core PUBLIC include)

target_link_libraries(hockey_core
    PUBLIC
        SDL3::SDL3
        spdlog::spdlog
        fmt::fmt
        tomlplusplus::tomlplusplus
)

if(WIN32)
    target_compile_definitions(hockey_core PUBLIC HK_PLATFORM_WINDOWS=1 HK_PLATFORM_LINUX=0)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hockey_core PUBLIC HK_PLATFORM_WINDOWS=0 HK_PLATFORM_LINUX=1)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_compile_definitions(hockey_core PUBLIC HK_CONFIG_DEBUG=$<CONFIG:Debug>)

if(MSVC)
    target_compile_options(hockey_core PRIVATE /W4)
else()
    target_compile_options(hockey_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_core PRIVATE /WX)
    else()
        target_compile_options(hockey_core PRIVATE -Werror)
    endif()
endif()
