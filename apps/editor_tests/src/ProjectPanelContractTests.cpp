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

std::size_t CountOccurrences(const std::string& text, const char* needle) {
    std::size_t count = 0;
    std::size_t cursor = 0;
    const std::string pattern{needle};
    while ((cursor = text.find(pattern, cursor)) != std::string::npos) {
        ++count;
        cursor += pattern.size();
    }
    return count;
}

} // namespace

void RunProjectPanelContractTests() {
    HockeyTest::BeginSuite("ProjectPanelContractTests");

    const std::string header = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/ProjectPanel.hpp");
    const std::string source = ReadProjectFile("libs/editor/src/Panels/ProjectPanel.cpp");

    HK_CHECK_MSG(Contains(header, "enum class ProjectSearchScope"), "Project panel tracks Unity-style search scope");
    HK_CHECK_MSG(Contains(header, "m_SelectedFolder"), "Project panel stores selected virtual folder");
    HK_CHECK_MSG(Contains(header, "m_SelectedAssetId"), "Project panel selection is cooked asset id based");
    HK_CHECK_MSG(Contains(header, "m_IconSize"), "Project panel stores icon/list view size");
    HK_CHECK_MSG(Contains(header, "m_TypeFilter"), "Project panel stores a cooked asset type filter");

    HK_CHECK_MSG(Contains(source, "DrawFolderTree"), "Project panel has a dedicated left folder tree");
    HK_CHECK_MSG(Contains(source, "DrawFolderContents"), "Project panel has a dedicated right contents pane");
    HK_CHECK_MSG(Contains(source, "DrawStatusStrip"), "Project panel has a bottom path/status strip");
    HK_CHECK_MSG(Contains(source, "DrawViewSizeSlider"), "Project panel has a bottom-right icon size slider");
    HK_CHECK_MSG(Contains(source, "Import All & Cook"), "Project toolbar exposes import plus cook");
    HK_CHECK_MSG(Contains(source, "Recook Dirty"), "Project toolbar keeps dirty recook as maintenance action");
    HK_CHECK_MSG(Contains(source, "Selected Root"), "Project toolbar exposes selected-root search scope");
    HK_CHECK_MSG(Contains(source, "Selected Folder"), "Project toolbar exposes selected-folder search scope");
    HK_CHECK_MSG(Contains(source, "Texture/Image"), "Project toolbar exposes texture/image type filter");
    HK_CHECK_MSG(Contains(source, "Reimport & Recook"), "Project asset context menu recooks through cooked selection");
    HK_CHECK_MSG(Contains(source, "Reveal Source"), "Project asset context menu can reveal source");
    HK_CHECK_MSG(Contains(source, "Reveal Cooked"), "Project asset context menu can reveal cooked output");
    HK_CHECK_MSG(Contains(source, "m_Browser.Entries") && Contains(source, "AssetDatabase"),
                 "Project panel renders from cooked ProjectBrowser entries");
    HK_CHECK_MSG(Contains(source, "CookAllDirty()"), "Project panel import/edit flows cook dirty assets");
    HK_CHECK_MSG(Contains(source, "DrawMaterialEditor"), "Project panel keeps material editor entry point");
    HK_CHECK_MSG(Contains(source, "ApplyMaterialPreview"), "Project panel keeps live material preview entry point");
    HK_CHECK_MSG(CountOccurrences(source, "EditorTooltip::ForLastItem") >= 18,
                 "New Project panel controls expose hover text");
}
