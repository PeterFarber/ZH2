#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/PanelManager.hpp"

using namespace Hockey;

namespace {

class DummyPanel final : public Panel {
public:
    explicit DummyPanel(std::string name, bool openByDefault = true) : Panel(std::move(name), openByDefault) {}

    void OnImGui(EditorContext&) override {}
};

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunEditorLayoutPersistenceTests() {
    HockeyTest::BeginSuite("EditorLayoutPersistenceTests");

    {
        PanelManager panels;
        panels.AddPanel<DummyPanel>("Hierarchy", true);
        panels.AddPanel<DummyPanel>("Console", true);
        panels.AddPanel<DummyPanel>("Project Settings", false);

        EditorSettings settings;
        settings.SetPanelOpen("Hierarchy", true);
        settings.SetPanelOpen("Console", false);

        panels.ApplyPanelOpenStates(settings);

        Panel* hierarchy = panels.FindPanel("Hierarchy");
        Panel* console = panels.FindPanel("Console");
        Panel* projectSettings = panels.FindPanel("Project Settings");

        HK_CHECK_MSG(hierarchy != nullptr && hierarchy->IsOpen(), "known open panel stays open");
        HK_CHECK_MSG(console != nullptr && !console->IsOpen(), "known closed panel is closed");
        HK_CHECK_MSG(projectSettings != nullptr && !projectSettings->IsOpen(),
                     "missing saved panel uses its default-open value");
    }

    {
        PanelManager panels;
        panels.AddPanel<DummyPanel>("Hierarchy", true);
        panels.AddPanel<DummyPanel>("Console", true);
        panels.SetPanelOpen("Console", false);

        const std::vector<EditorPanelOpenState> captured = panels.CapturePanelOpenStates();
        HK_CHECK_EQ(captured.size(), static_cast<std::size_t>(2));
        HK_CHECK_EQ(captured[0].name, std::string("Hierarchy"));
        HK_CHECK_EQ(captured[0].open, true);
        HK_CHECK_EQ(captured[1].name, std::string("Console"));
        HK_CHECK_EQ(captured[1].open, false);
    }

    {
        PanelManager panels;
        panels.AddPanel<DummyPanel>("Hierarchy", true);
        panels.AddPanel<DummyPanel>("Project Settings", false);
        panels.SetPanelOpen("Hierarchy", false);
        panels.SetPanelOpen("Project Settings", true);

        panels.ResetPanelOpenStates();

        HK_CHECK_MSG(panels.FindPanel("Hierarchy")->IsOpen(), "reset reopens default-open panels");
        HK_CHECK_MSG(!panels.FindPanel("Project Settings")->IsOpen(), "reset closes default-closed panels");
    }

    {
        const std::string appHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorApp.hpp");
        const std::string appSource = ReadProjectFile("libs/editor/src/EditorApp.cpp");
        const std::string menuSource = ReadProjectFile("libs/editor/src/MainMenuBar.cpp");
        const std::string imguiSource = ReadProjectFile("libs/editor/src/ImGui/ImGuiLayer.cpp");

        HK_CHECK_MSG(Contains(appHeader, "void SaveLayout();"), "EditorApp exposes SaveLayout");
        HK_CHECK_MSG(Contains(appHeader, "void ResetLayout();"), "EditorApp exposes ResetLayout");
        HK_CHECK_MSG(Contains(appSource, "ApplyPanelOpenStates(m_Context.settings)"),
                     "EditorApp applies saved panel state after registration");
        HK_CHECK_MSG(Contains(appSource, "SaveLayout();") && Contains(appSource, "m_ImGuiLayer.Shutdown();"),
                     "EditorApp saves layout before destroying ImGui");
        HK_CHECK_MSG(Contains(menuSource, "\"Save Layout\""), "View menu exposes Save Layout");
        HK_CHECK_MSG(Contains(menuSource, "\"Reset Layout\""), "View menu still exposes Reset Layout");
        HK_CHECK_MSG(Contains(imguiSource, "ImGui::SaveIniSettingsToDisk"),
                     "ImGuiLayer explicitly saves layout.ini");
    }
}
