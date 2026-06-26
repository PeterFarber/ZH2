cmake_minimum_required(VERSION 3.25)

project(HockeyGame
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always emit compile_commands.json for clangd / tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clangd is configured to look in <workspace>/build, but each preset writes its
# database to <workspace>/build/<preset>/. Mirror it up to <workspace>/build so
# clangd finds it regardless of the active preset. Best-effort and cross-platform
# (symlink on Linux/macOS, copy fallback on Windows); never aborts configure.
if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT CMAKE_BINARY_DIR STREQUAL "${CMAKE_SOURCE_DIR}/build")
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
        SYMBOLIC COPY_ON_ERROR RESULT _hk_cc_link_result)
endif()

option(HOCKEY_BUILD_TESTS "Build Hockey engine tests" ON)
option(HOCKEY_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_subdirectory(libs/core)
add_subdirectory(libs/ecs)
add_subdirectory(libs/assets)
add_subdirectory(libs/renderer)
add_subdirectory(libs/ui)
add_subdirectory(libs/physics)
add_subdirectory(libs/gameplay)
add_subdirectory(libs/editor)

add_subdirectory(apps/game_client)
add_subdirectory(apps/map_editor)
add_subdirectory(apps/dedicated_server)
add_subdirectory(apps/asset_tool)
add_subdirectory(apps/shader_tool)

if(HOCKEY_BUILD_TESTS)
    add_subdirectory(apps/core_tests)
    add_subdirectory(apps/ecs_tests)
    add_subdirectory(apps/renderer_tests)
    add_subdirectory(apps/ui_tests)
    add_subdirectory(apps/editor_tests)
    add_subdirectory(apps/asset_tests)
    add_subdirectory(apps/physics_tests)
    add_subdirectory(apps/gameplay_tests)
endif()

# Phase 3/4 architectural guard: the dedicated server must remain headless and
# must never link the renderer or any editor/ImGui UI. Fail configuration if a
# forbidden dependency appears.
if(TARGET HockeyDedicatedServer)
    get_target_property(_hk_server_libs HockeyDedicatedServer LINK_LIBRARIES)
    if(_hk_server_libs AND "hockey_renderer" IN_LIST _hk_server_libs)
        message(FATAL_ERROR "HockeyDedicatedServer must not link hockey_renderer (headless server invariant).")
    endif()
    if(_hk_server_libs AND "hockey_editor" IN_LIST _hk_server_libs)
        message(FATAL_ERROR "HockeyDedicatedServer must not link hockey_editor (headless server invariant).")
    endif()
    if(_hk_server_libs AND "imgui::imgui" IN_LIST _hk_server_libs)
        message(FATAL_ERROR "HockeyDedicatedServer must not link ImGui (headless server invariant).")
    endif()
    if(_hk_server_libs AND "hockey_ui" IN_LIST _hk_server_libs)
        message(FATAL_ERROR "HockeyDedicatedServer must not link hockey_ui (headless server invariant).")
    endif()
    if(_hk_server_libs AND "RmlUi::RmlUi" IN_LIST _hk_server_libs)
        message(FATAL_ERROR "HockeyDedicatedServer must not link RmlUi (headless server invariant).")
    endif()
endif()

# Phase 4 architectural guard: the game client should not link the editor UI.
if(TARGET HockeyGameClient)
    get_target_property(_hk_client_libs HockeyGameClient LINK_LIBRARIES)
    if(_hk_client_libs AND "hockey_editor" IN_LIST _hk_client_libs)
        message(FATAL_ERROR "HockeyGameClient must not link hockey_editor.")
    endif()
endif()

# Phase 5 architectural guard: hockey_assets owns CPU-side content only. It must
# depend on hockey_core alone and never pull in renderer/editor/ecs/physics/
# networking/gameplay (the renderer and editor depend on assets, not vice versa).
if(TARGET hockey_assets)
    get_target_property(_hk_assets_libs hockey_assets LINK_LIBRARIES)
    foreach(_forbidden hockey_renderer hockey_editor hockey_ecs hockey_physics hockey_networking hockey_gameplay)
        if(_hk_assets_libs AND "${_forbidden}" IN_LIST _hk_assets_libs)
            message(FATAL_ERROR "hockey_assets must not link ${_forbidden} (asset library owns CPU-side content only).")
        endif()
    endforeach()
endif()

# Phase 6 architectural guard: hockey_physics depends only on hockey_core and
# hockey_ecs (plus its private Jolt dependency). It must never link the renderer,
# editor, networking or gameplay, and must never know about Vulkan/ImGui.
if(TARGET hockey_physics)
    get_target_property(_hk_physics_libs hockey_physics LINK_LIBRARIES)
    foreach(_forbidden hockey_renderer hockey_editor hockey_networking hockey_gameplay imgui::imgui)
        if(_hk_physics_libs AND "${_forbidden}" IN_LIST _hk_physics_libs)
            message(FATAL_ERROR "hockey_physics must not link ${_forbidden} (physics owns simulation only).")
        endif()
    endforeach()
endif()

# Phase 7 architectural guard: hockey_gameplay owns hockey rules and simulation
# only. It must stay independent from renderer/editor/networking/platform UI.
if(TARGET hockey_gameplay)
    get_target_property(_hk_gameplay_libs hockey_gameplay LINK_LIBRARIES)
    foreach(_forbidden hockey_renderer hockey_editor hockey_networking SDL3::SDL3 Vulkan::Vulkan imgui::imgui)
        if(_hk_gameplay_libs AND "${_forbidden}" IN_LIST _hk_gameplay_libs)
            message(FATAL_ERROR "hockey_gameplay must not link ${_forbidden} (gameplay must stay transport/render/UI independent).")
        endif()
    endforeach()
endif()
