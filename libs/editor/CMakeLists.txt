# hockey_editor: shared Unity-style map editor library. Depends on
# hockey_core, hockey_ecs, hockey_renderer and hockey_assets (Project panel,
# inspector asset fields and hot reload) plus the editor-only UI dependencies
# (Dear ImGui, ImGuizmo, native file dialogs). The dedicated server must never
# link this target (it would pull in ImGui/renderer).

find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)

add_library(hockey_editor STATIC)

target_sources(hockey_editor
    PRIVATE
        src/EditorApp.cpp
        src/EditorCamera.cpp
        src/EditorClientPreview.cpp
        src/EditorGameplayPreview.cpp
        src/EditorPhysicsPreview.cpp
        src/EditorSettings.cpp
        src/SceneViewOverlay.cpp
        src/SceneWorkflow.cpp
        src/FileDialog.cpp
        src/Dockspace.cpp
        src/MainMenuBar.cpp
        src/Packaging/PackageProfile.cpp
        src/Packaging/PackageCommand.cpp
        src/Toolbar.cpp
        src/Panel.cpp
        src/PanelManager.cpp
        src/Selection.cpp
        src/UndoRedo.cpp
        src/EditorCommands.cpp
        src/EditorTransformOperations.cpp
        src/Clipboard.cpp
        src/Gizmos/CameraLightGizmo.cpp
        src/Gizmos/GridGizmo.cpp
        src/Gizmos/PhysicsGizmo.cpp
        src/Gizmos/SelectionOutline.cpp
        src/Gizmos/TransformGizmo.cpp
        src/Logging/EditorConsoleSink.cpp
        src/Project/FileTypeRegistry.cpp
        src/Project/ProjectBrowser.cpp
        src/Project/EditorAssetPreview.cpp
        src/Project/EditorAssetPreviewRenderer.cpp
        src/Tools/EditorTool.cpp
        src/Tools/ToolManager.cpp
        src/Tools/TransformTools.cpp
        src/Tools/LightTool.cpp
        src/Tools/HockeyTools.cpp
        src/Tools/EditorTools.cpp
        src/Inspector/FieldDrawers.cpp
        src/Inspector/AssetInspector.cpp
        src/Inspector/ComponentInspector.cpp
        src/Inspector/ComponentAddMenu.cpp
        src/Inspector/MaterialPicker.cpp
        src/Panels/HierarchyPanel.cpp
        src/Panels/InspectorPanel.cpp
        src/Panels/SceneViewportPanel.cpp
        src/Panels/GameViewportPanel.cpp
        src/Panels/ClientPreviewPanel.cpp
        src/Panels/ClientFlowPanel.cpp
        src/Panels/GameplayTuningPanel.cpp
        src/Panels/ProjectPanel.cpp
        src/Panels/ConsolePanel.cpp
        src/Panels/PropertiesPanel.cpp
        src/Panels/StatsPanel.cpp
        src/Panels/SceneValidationPanel.cpp
        src/Panels/PrefabPanel.cpp
        src/Panels/ProjectSettingsPanel.cpp
        src/Panels/PackagePanel.cpp
        src/ImGui/EditorTooltip.cpp
        src/ImGui/EditorTheme.cpp
        src/ImGui/EditorIcons.cpp
        src/ImGui/ImGuiRendererBridge.cpp
        src/ImGui/ImGuiLayer.cpp
        # Dear ImGui's Vulkan backend, vendored so we can build it against volk
        # (the renderer's loader). See vendor/imgui_backend/README.md.
        vendor/imgui_backend/imgui_impl_vulkan.cpp
)

target_include_directories(hockey_editor
    PUBLIC include
    # The vendored Vulkan backend header must shadow any vcpkg-provided copy, so
    # keep it on the (higher priority) -I path rather than the -isystem path.
    PRIVATE
        vendor/imgui_backend
        vendor/icon_font_cpp_headers
)

# Route Dear ImGui's Vulkan backend through volk: VK_NO_PROTOTYPES makes the
# Vulkan headers declare entry points as pointers, and IMGUI_IMPL_VULKAN_USE_VOLK
# makes the backend pull in <volk.h> and share the renderer's loaded function
# pointers. Without this the backend would call the loader directly and collide
# with volk's global symbols.
target_compile_definitions(hockey_editor PRIVATE VK_NO_PROTOTYPES IMGUI_IMPL_VULKAN_USE_VOLK)

# hockey_renderer/hockey_ecs/hockey_core are PUBLIC because editor headers
# expose Renderer/Scene/Entity types. ImGui is PUBLIC because panels include
# imgui.h. Vulkan/file-dialog usage stays inside .cpp files (PRIVATE).
target_link_libraries(hockey_editor
    PUBLIC
        hockey_core
        hockey_ecs
        hockey_renderer
        hockey_ui
        hockey_assets
        hockey_physics
        hockey_gameplay
        imgui::imgui
    PRIVATE
        imguizmo::imguizmo
        nfd::nfd
        volk::volk_headers
        Vulkan::Headers
)

if(MSVC)
    target_compile_options(hockey_editor PRIVATE /W4)
else()
    target_compile_options(hockey_editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_editor PRIVATE /WX)
    else()
        target_compile_options(hockey_editor PRIVATE -Werror)
    endif()
endif()

# The vendored third-party backend is exempt from our strict warning set.
if(MSVC)
    set_source_files_properties(vendor/imgui_backend/imgui_impl_vulkan.cpp
        PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(vendor/imgui_backend/imgui_impl_vulkan.cpp
        PROPERTIES COMPILE_OPTIONS "-w")
endif()
