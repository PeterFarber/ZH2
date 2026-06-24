#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <imgui.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

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

void RunEditorTooltipContractTests() {
    HockeyTest::BeginSuite("EditorTooltipContractTests");

    const ImGuiHoveredFlags flags = Hockey::EditorTooltip::DefaultHoverFlags();
    HK_CHECK_MSG((flags & ImGuiHoveredFlags_AllowWhenDisabled) != 0,
                 "Editor tooltips stay available for disabled controls");
    HK_CHECK_MSG((flags & ImGuiHoveredFlags_DelayShort) != 0, "Editor tooltips use a short hover delay");

    Hockey::EditorTooltip::ForLastItem(nullptr);
    Hockey::EditorTooltip::ForLastItem("");
    Hockey::EditorTooltip::ForLastItem(std::string_view{});
    HK_CHECK_MSG(true, "EditorTooltip safely ignores null and empty text");

    const std::string editorCMake = ReadProjectFile("libs/editor/CMakeLists.txt");
    const std::string helperHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/ImGui/EditorTooltip.hpp");
    const std::string helperSource = ReadProjectFile("libs/editor/src/ImGui/EditorTooltip.cpp");
    const std::string toolbar = ReadProjectFile("libs/editor/src/Toolbar.cpp");
    const std::string projectSettings = ReadProjectFile("libs/editor/src/Panels/ProjectSettingsPanel.cpp");
    const std::string projectPanel = ReadProjectFile("libs/editor/src/Panels/ProjectPanel.cpp");
    const std::string sceneViewOverlay = ReadProjectFile("libs/editor/src/SceneViewOverlay.cpp");
    const std::string hierarchyPanel = ReadProjectFile("libs/editor/src/Panels/HierarchyPanel.cpp");
    const std::string inspectorPanel = ReadProjectFile("libs/editor/src/Panels/InspectorPanel.cpp");
    const std::string prefabPanel = ReadProjectFile("libs/editor/src/Panels/PrefabPanel.cpp");
    const std::string statsPanel = ReadProjectFile("libs/editor/src/Panels/StatsPanel.cpp");
    const std::string consolePanel = ReadProjectFile("libs/editor/src/Panels/ConsolePanel.cpp");
    const std::string sceneValidationPanel = ReadProjectFile("libs/editor/src/Panels/SceneValidationPanel.cpp");

    HK_CHECK_MSG(Contains(editorCMake, "src/ImGui/EditorTooltip.cpp"), "hockey_editor builds EditorTooltip.cpp");
    HK_CHECK_MSG(Contains(helperHeader, "namespace Hockey::EditorTooltip"), "shared tooltip helper namespace exists");
    HK_CHECK_MSG(Contains(helperHeader, "ImGuiHoveredFlags DefaultHoverFlags()"), "default hover flags API exists");
    HK_CHECK_MSG(Contains(helperHeader, "void ForLastItem(const char* text)"), "C-string tooltip API exists");
    HK_CHECK_MSG(Contains(helperHeader, "void ForLastItem(std::string_view text)"),
                 "string_view tooltip API exists");
    HK_CHECK_MSG(Contains(helperSource, "ImGuiHoveredFlags_DelayShort"), "helper uses short delay flag");
    HK_CHECK_MSG(Contains(helperSource, "ImGuiHoveredFlags_AllowWhenDisabled"),
                 "helper supports disabled item hover");
    HK_CHECK_MSG(Contains(helperSource, "ImGui::IsItemHovered(DefaultHoverFlags())"),
                 "helper applies the shared hover flags");

    HK_CHECK_MSG(Contains(toolbar, "Hockey/Editor/ImGui/EditorTooltip.hpp"), "Toolbar includes tooltip helper");
    const std::size_t toolbarTooltipAffordances = CountOccurrences(toolbar, "EditorTooltip::ForLastItem") +
                                                  CountOccurrences(toolbar, "EditorIconButton(") +
                                                  CountOccurrences(toolbar, "EditorIconToggleButton(");
    HK_CHECK_MSG(toolbarTooltipAffordances >= 4,
                 "Toolbar controls expose useful hover text");
    HK_CHECK_MSG(Contains(projectSettings, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Project Settings includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(projectSettings, "EditorTooltip::ForLastItem") >= 5,
                 "Project Settings draw helpers route controls through the tooltip helper");
    HK_CHECK_MSG(Contains(projectSettings, "Internal render resolution multiplier"),
                 "Project Settings renderer controls expose hover text");
    HK_CHECK_MSG(Contains(projectSettings, "Caps how many scene lights the renderer submits"),
                 "Project Settings lighting budget controls expose hover text");
    HK_CHECK_MSG(Contains(projectSettings, "Directional shadow atlas resolution"),
                 "Project Settings shadow atlas controls expose hover text");
    HK_CHECK_MSG(Contains(projectSettings, "Changing render scale changes how sharp the scene appears"),
                 "Project Settings render scale tooltip explains scene sharpness");
    HK_CHECK_MSG(Contains(projectSettings, "Directional atlas controls how much texel detail the sun shadow map has"),
                 "Project Settings directional atlas tooltip explains shadow detail");
    HK_CHECK_MSG(Contains(projectSettings, "Cascade count controls how far directional shadows stay detailed"),
                 "Project Settings cascade tooltip explains camera-distance shadow detail");
    HK_CHECK_MSG(Contains(projectSettings, "Contact shadows add small grounding shadows under skates"),
                 "Project Settings contact shadow tooltip explains object grounding");
    HK_CHECK_MSG(Contains(projectSettings, "Ice reflection changes how clearly lights, players, and boards reflect"),
                 "Project Settings ice reflection tooltip explains rink reflection impact");
    HK_CHECK_MSG(Contains(projectSettings, "Physics fixed delta controls how often preview collisions"),
                 "Project Settings physics tooltip explains simulation cadence");
    HK_CHECK_MSG(Contains(projectSettings, "Startup scene is the scene loaded when players launch the build"),
                 "Project Settings startup scene tooltip explains build behavior");
    HK_CHECK_MSG(Contains(projectSettings, "Validate on load checks the scene for missing required gameplay objects"),
                 "Project Settings validation tooltip explains scene safety");
    HK_CHECK_MSG(Contains(projectSettings, "Target player count controls how many skaters and goalies"),
                 "Project Settings gameplay tooltip explains roster expectations");
    HK_CHECK_MSG(Contains(projectPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Project Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(projectPanel, "EditorTooltip::ForLastItem") >= 12,
                 "Project Panel asset and material actions expose hover text");
    HK_CHECK_MSG(!Contains(projectPanel, "ImGui::SetTooltip"), "Project Panel routes touched tooltips through helper");
    HK_CHECK_MSG(Contains(sceneViewOverlay, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Scene View overlay includes tooltip helper");
    HK_CHECK_MSG(!Contains(sceneViewOverlay, "ImGui::SetTooltip"),
                 "Scene View overlay routes its local helper through EditorTooltip");

    HK_CHECK_MSG(Contains(hierarchyPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Hierarchy Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(hierarchyPanel, "EditorTooltip::ForLastItem") >= 8,
                 "Hierarchy Panel exposes hover text for tree and context menu actions");
    HK_CHECK_MSG(Contains(hierarchyPanel, "Drag to reorder or parent this entity"),
                 "Hierarchy entity rows describe drag/drop organization");

    HK_CHECK_MSG(Contains(inspectorPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Inspector Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(inspectorPanel, "EditorTooltip::ForLastItem") >= 6,
                 "Inspector header controls expose hover text");
    HK_CHECK_MSG(!Contains(inspectorPanel, "ImGui::SetTooltip"),
                 "Inspector Panel routes touched tooltips through EditorTooltip");

    HK_CHECK_MSG(Contains(prefabPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Prefab Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(prefabPanel, "EditorTooltip::ForLastItem") >= 7,
                 "Prefab Panel workflow buttons expose hover text");
    HK_CHECK_MSG(Contains(prefabPanel, "Creates a prefab asset from the selected entity"),
                 "Prefab creation explains what will be written");

    HK_CHECK_MSG(Contains(statsPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Stats Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(statsPanel, "EditorTooltip::ForLastItem") >= 3,
                 "Stats Panel sections explain the performance counters");

    HK_CHECK_MSG(Contains(consolePanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Console Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(consolePanel, "EditorTooltip::ForLastItem") >= 8,
                 "Console Panel filters and actions expose hover text");

    HK_CHECK_MSG(Contains(sceneValidationPanel, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Scene Validation Panel includes tooltip helper");
    HK_CHECK_MSG(CountOccurrences(sceneValidationPanel, "EditorTooltip::ForLastItem") >= 4,
                 "Scene Validation Panel actions and issue rows expose hover text");
}
