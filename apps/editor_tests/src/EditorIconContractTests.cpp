#include "Test.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "Hockey/Core/Paths.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

bool ProjectFileExists(const char* relativePath) {
    return std::filesystem::exists(Hockey::Paths::Get().root / relativePath);
}

std::size_t CountOccurrences(const std::string& text, const char* needle) {
    std::size_t count = 0;
    std::size_t cursor = 0;
    const std::string_view pattern{needle};
    while ((cursor = text.find(pattern, cursor)) != std::string::npos) {
        ++count;
        cursor += pattern.size();
    }
    return count;
}

} // namespace

void RunEditorIconContractTests() {
    HockeyTest::BeginSuite("EditorIconContractTests");

    const std::string editorCMake = ReadProjectFile("libs/editor/CMakeLists.txt");
    const std::string testCMake = ReadProjectFile("apps/editor_tests/CMakeLists.txt");
    const std::string testMain = ReadProjectFile("apps/editor_tests/src/EditorTestsMain.cpp");
    const std::string imguiLayerHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/ImGui/ImGuiLayer.hpp");
    const std::string imguiLayerSource = ReadProjectFile("libs/editor/src/ImGui/ImGuiLayer.cpp");
    const std::string iconsHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/ImGui/EditorIcons.hpp");
    const std::string iconsSource = ReadProjectFile("libs/editor/src/ImGui/EditorIcons.cpp");
    const std::string toolbar = ReadProjectFile("libs/editor/src/Toolbar.cpp");
    const std::string menu = ReadProjectFile("libs/editor/src/MainMenuBar.cpp");
    const std::string projectPanel = ReadProjectFile("libs/editor/src/Panels/ProjectPanel.cpp");
    const std::string hierarchyPanel = ReadProjectFile("libs/editor/src/Panels/HierarchyPanel.cpp");
    const std::string inspectorPanel = ReadProjectFile("libs/editor/src/Panels/InspectorPanel.cpp");
    const std::string assetInspector = ReadProjectFile("libs/editor/src/Inspector/AssetInspector.cpp");
    const std::string fieldDrawers = ReadProjectFile("libs/editor/src/Inspector/FieldDrawers.cpp");
    const std::string sceneOverlay = ReadProjectFile("libs/editor/src/SceneViewOverlay.cpp");

    HK_CHECK_MSG(ProjectFileExists("data/editor/fonts/fontawesome-free/fa-solid-900.ttf"),
                 "Font Awesome Free Solid font is vendored");
    HK_CHECK_MSG(ProjectFileExists("data/editor/fonts/fontawesome-free/LICENSE.txt"),
                 "Font Awesome Free license is vendored");
    HK_CHECK_MSG(ProjectFileExists("data/editor/fonts/fontawesome-free/README.md"),
                 "Font Awesome Free source/version notes are vendored");
    HK_CHECK_MSG(ProjectFileExists("libs/editor/vendor/icon_font_cpp_headers/IconsFontAwesome.h"),
                 "IconFontCppHeaders Font Awesome header is vendored");
    HK_CHECK_MSG(ProjectFileExists("libs/editor/vendor/icon_font_cpp_headers/licence.txt"),
                 "IconFontCppHeaders license is vendored");
    HK_CHECK_MSG(ProjectFileExists("libs/editor/vendor/icon_font_cpp_headers/README.md"),
                 "IconFontCppHeaders source/version notes are vendored");

    HK_CHECK_MSG(Contains(editorCMake, "src/ImGui/EditorIcons.cpp"), "hockey_editor builds EditorIcons.cpp");
    HK_CHECK_MSG(Contains(editorCMake, "vendor/icon_font_cpp_headers"),
                 "hockey_editor includes the icon-font header directory privately");
    HK_CHECK_MSG(Contains(testCMake, "src/EditorIconContractTests.cpp"),
                 "HockeyEditorTests builds icon contract tests");
    HK_CHECK_MSG(Contains(testMain, "RunEditorIconContractTests"),
                 "Editor icon contract suite is registered");

    HK_CHECK_MSG(Contains(imguiLayerHeader, "IconFontLoaded() const"), "ImGuiLayer exposes icon-font load state");
    HK_CHECK_MSG(Contains(imguiLayerHeader, "m_IconFontLoaded"), "ImGuiLayer stores icon-font load state");
    HK_CHECK_MSG(Contains(imguiLayerSource, "IconsFontAwesome.h"), "ImGuiLayer includes icon codepoint header");
    HK_CHECK_MSG(Contains(imguiLayerSource, "LoadEditorFonts"), "ImGuiLayer has an editor font loader");
    HK_CHECK_MSG(Contains(imguiLayerSource, "Paths::DataFile(\"editor/fonts/fontawesome-free/fa-solid-900.ttf\")"),
                 "ImGuiLayer resolves Font Awesome through project data paths");
    HK_CHECK_MSG(Contains(imguiLayerSource, "MergeMode = true"), "ImGuiLayer merges the icon font into the UI font");
    HK_CHECK_MSG(Contains(imguiLayerSource, "GlyphMinAdvanceX"), "ImGuiLayer gives icons stable advance width");
    HK_CHECK_MSG(Contains(imguiLayerSource, "HK_EDITOR_WARN"), "ImGuiLayer warns when icon font loading falls back");

    HK_CHECK_MSG(Contains(iconsHeader, "enum class EditorIcon"), "EditorIcon enum exists");
    HK_CHECK_MSG(Contains(iconsHeader, "EditorIconGlyph(EditorIcon icon)"), "EditorIcon glyph API exists");
    HK_CHECK_MSG(Contains(iconsHeader, "EditorIconLabel(EditorIcon icon"), "EditorIcon label API exists");
    HK_CHECK_MSG(Contains(iconsHeader, "EditorIconButton(EditorIcon icon"), "EditorIcon button API exists");
    HK_CHECK_MSG(Contains(iconsHeader, "EditorIconToggleButton(EditorIcon icon"), "EditorIcon toggle button API exists");
    HK_CHECK_MSG(Contains(iconsHeader, "EditorIconForAssetType(EditorFileType type)"),
                 "EditorIcon asset type mapping API exists");
    HK_CHECK_MSG(Contains(iconsSource, "ICON_FA_PLAY") && Contains(iconsSource, "ICON_FA_FLOPPY_DISK") &&
                     Contains(iconsSource, "ICON_FA_MAGNIFYING_GLASS"),
                 "high-traffic action icons are mapped");
    HK_CHECK_MSG(Contains(iconsSource, "ICON_FA_FOLDER") && Contains(iconsSource, "ICON_FA_CUBE") &&
                     Contains(iconsSource, "ICON_FA_EYE") && Contains(iconsSource, "ICON_FA_LOCK"),
                 "asset, hierarchy, and state icons are mapped");

    HK_CHECK_MSG(Contains(toolbar, "Hockey/Editor/ImGui/EditorIcons.hpp"),
                 "Toolbar includes editor icon helpers");
    HK_CHECK_MSG(CountOccurrences(toolbar, "EditorIcon") >= 3, "Toolbar uses editor icon helpers");
    HK_CHECK_MSG(Contains(menu, "EditorIconLabel"), "Main menu uses icon labels");
    HK_CHECK_MSG(Contains(projectPanel, "EditorIconForAssetType"), "Project panel uses asset-type icon mapping");
    HK_CHECK_MSG(!Contains(projectPanel, "TypeGlyph("), "Project panel no longer uses local type glyph strings");
    HK_CHECK_MSG(Contains(hierarchyPanel, "EditorIcon::Visible") && Contains(hierarchyPanel, "EditorIcon::Locked"),
                 "Hierarchy visibility and pickability controls use editor icons");
    HK_CHECK_MSG(Contains(inspectorPanel, "EditorIcon::Prefab") || Contains(inspectorPanel, "EditorIcon::Mesh"),
                 "Inspector header uses an editor icon affordance");
    HK_CHECK_MSG(Contains(assetInspector, "EditorIcon::Save") && Contains(assetInspector, "EditorIcon::Refresh"),
                 "Asset inspector actions use editor icons");
    HK_CHECK_MSG(Contains(fieldDrawers, "EditorIcon::Clear") && Contains(fieldDrawers, "EditorIcon::Settings"),
                 "Field drawers use editor icons for picker and clear affordances");
    HK_CHECK_MSG(Contains(sceneOverlay, "EditorIcon::Select") && Contains(sceneOverlay, "EditorIcon::Move") &&
                     Contains(sceneOverlay, "EditorIcon::Rotate") && Contains(sceneOverlay, "EditorIcon::Scale"),
                 "Scene View overlay tool strip uses editor icons");
}
