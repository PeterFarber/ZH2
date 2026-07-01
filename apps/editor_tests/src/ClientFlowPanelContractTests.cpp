#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Project/FileTypeRegistry.hpp"

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

} // namespace

void RunClientFlowPanelContractTests() {
    HockeyTest::BeginSuite("ClientFlowPanelContracts");

    const Hockey::FileTypeInfo screen = Hockey::FileTypeRegistry::Classify("data/ui/screens/home.rml");
    HK_CHECK_EQ(screen.label, std::string("UI Screen"));
    HK_CHECK(screen.supported);

    const Hockey::FileTypeInfo theme = Hockey::FileTypeRegistry::Classify("data/ui/themes/hockey.rcss");
    HK_CHECK_EQ(theme.label, std::string("UI Theme"));
    HK_CHECK(theme.supported);

    const Hockey::FileTypeInfo flow = Hockey::FileTypeRegistry::Classify("data/ui/flows/default.clientflow.yaml");
    HK_CHECK_EQ(flow.label, std::string("Client Flow"));
    HK_CHECK(flow.supported);

    const std::string browser = ReadProjectFile("libs/editor/src/Project/ProjectBrowser.cpp");
    HK_CHECK_MSG(Contains(browser, "data/ui"), "Project browser includes runtime UI asset root");

    const std::string projectPanel = ReadProjectFile("libs/editor/src/Panels/ProjectPanel.cpp");
    HK_CHECK_MSG(Contains(projectPanel, "UI Screen"), "Project panel can create UI Screen assets");
    HK_CHECK_MSG(Contains(projectPanel, "UI Theme"), "Project panel can create UI Theme assets");
    HK_CHECK_MSG(Contains(projectPanel, "Client Flow"), "Project panel can create Client Flow assets");
    HK_CHECK_MSG(Contains(projectPanel, "EditorPanelNames::kClientFlow"), "Client flow files open the Client Flow panel");

    const std::string panelHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/ClientFlowPanel.hpp");
    HK_CHECK_MSG(Contains(panelHeader, "ClientFlowPanel"), "ClientFlowPanel header exists");

    const std::string panelSource = ReadProjectFile("libs/editor/src/Panels/ClientFlowPanel.cpp");
    HK_CHECK_MSG(Contains(panelSource, "Set As Client Startup"), "ClientFlowPanel can update client startup flow");
    HK_CHECK_MSG(Contains(panelSource, "Play Client"), "ClientFlowPanel can start Client Preview");
    HK_CHECK_MSG(Contains(panelSource, "ClientFlowSerializer"), "ClientFlowPanel uses ClientFlowSerializer");
    HK_CHECK_MSG(Contains(panelSource, "Loading Screen") && Contains(panelSource, "Lobby Screen") &&
                     Contains(panelSource, "Team Select") && Contains(panelSource, "Pause Menu") &&
                     Contains(panelSource, "Settings Screen") && Contains(panelSource, "Scoreboard") &&
                     Contains(panelSource, "End Match"),
                 "ClientFlowPanel exposes all screen document paths");
    HK_CHECK_MSG(Contains(panelSource, "Home Background") && Contains(panelSource, "Lobby Background") &&
                     Contains(panelSource, "Settings Background") && Contains(panelSource, "End Match Background"),
                 "ClientFlowPanel exposes optional background scene paths");
    HK_CHECK_MSG(Contains(panelSource, "Use Current Editor Scene") &&
                     Contains(panelSource, "useCurrentEditorSceneWhenPreviewing"),
                 "ClientFlowPanel edits the current-editor-scene preview toggle");
    HK_CHECK_MSG(Contains(panelSource, "ValidateClientFlow") && Contains(panelSource, "Missing file"),
                 "ClientFlowPanel validates client-flow fields before save/play");
    HK_CHECK_MSG(Contains(panelSource, "ImGui::TextUnformatted(label)") &&
                     Contains(panelSource, "ImGui::InputText(\"##path\"") &&
                     !Contains(panelSource, "ImGui::InputText(label"),
                 "ClientFlowPanel draws path labels before hidden-label inputs so labels do not clip at the right edge");
    HK_CHECK_MSG(Contains(projectPanel, "Open Source") && Contains(projectPanel, "OpenSourceFile"),
                 "Project panel can open RML/RCSS source files");
}
