#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

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

} // namespace

void RunEditorClientPreviewTests() {
    HockeyTest::BeginSuite("EditorClientPreview");

    const std::string cmake = ReadProjectFile("libs/editor/CMakeLists.txt");
    HK_CHECK_MSG(Contains(cmake, "hockey_ui"), "Editor links hockey_ui only for hosted Client Preview");
    HK_CHECK_MSG(Contains(cmake, "src/EditorClientPreview.cpp"), "EditorClientPreview source is part of hockey_editor");
    HK_CHECK_MSG(Contains(cmake, "src/Panels/ClientPreviewPanel.cpp"), "Client Preview panel is built");

    const std::string dockspace = ReadProjectFile("libs/editor/include/Hockey/Editor/Dockspace.hpp");
    HK_CHECK_MSG(Contains(dockspace, "kClientPreview"), "Dockspace exposes Client Preview panel name");

    const std::string appHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorApp.hpp");
    HK_CHECK_MSG(Contains(appHeader, "ToggleClientPreviewMode"), "EditorApp exposes Play Client action");
    HK_CHECK_MSG(Contains(appHeader, "EditorClientPreview"), "EditorApp owns EditorClientPreview");

    const std::string appSource = ReadProjectFile("libs/editor/src/EditorApp.cpp");
    HK_CHECK_MSG(Contains(appSource, "AddPanel<ClientPreviewPanel>"), "EditorApp registers Client Preview panel");
    HK_CHECK_MSG(Contains(appSource, "EditorPanelNames::kClientPreview"), "Play Client focuses Client Preview panel");
    HK_CHECK_MSG(Contains(appSource, "TogglePlaytestMode"), "Direct scene simulation path remains present");

    const std::string toolbar = ReadProjectFile("libs/editor/src/Toolbar.cpp");
    HK_CHECK_MSG(Contains(toolbar, "Play Client"), "Toolbar exposes Play Client");
    HK_CHECK_MSG(Contains(toolbar, "Simulate Scene"), "Toolbar keeps direct scene simulation distinct");

    const std::string previewHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorClientPreview.hpp");
    HK_CHECK_MSG(Contains(previewHeader, "Start") && Contains(previewHeader, "Stop") && Contains(previewHeader, "RenderToTarget"),
                 "EditorClientPreview exposes start/stop/render lifecycle");
}
