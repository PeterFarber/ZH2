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

    HK_CHECK_MSG(Contains(header, "enum class ProjectSection"), "Project panel tracks fixed cooked sections");
    HK_CHECK_MSG(Contains(header, "m_SelectedAssetId"), "Project panel selection is cooked asset id based");
    HK_CHECK_MSG(Contains(header, "m_IconSize"), "Project panel stores icon/list view size");
    HK_CHECK_MSG(Contains(header, "m_TypeFilter"), "Project panel stores a cooked asset type filter");

    HK_CHECK_MSG(Contains(source, "DrawSectionList"), "Project panel has a dedicated left cooked section list");
    HK_CHECK_MSG(Contains(source, "SectionListWidth"), "Project panel computes section list width from its labels");
    HK_CHECK_MSG(Contains(source, "ImGui::CalcTextSize"), "Project section width measures actual rendered label text");
    HK_CHECK_MSG(!Contains(source, "* 0.32f"), "Project section width is fixed to content instead of viewport percentage");
    HK_CHECK_MSG(Contains(source, "DrawFolderContents"), "Project panel has a dedicated right contents pane");
    HK_CHECK_MSG(Contains(source, "DrawStatusStrip"), "Project panel has a bottom path/status strip");
    HK_CHECK_MSG(Contains(source, "DrawViewSizeSlider"), "Project panel has a bottom-right icon size slider");
    HK_CHECK_MSG(Contains(source, "All") && Contains(source, "Models") && Contains(source, "Prefabs") &&
                     Contains(source, "Scenes") && Contains(source, "Textures"),
                 "Project panel exposes default cooked sections");
    HK_CHECK_MSG(Contains(source, "Import All & Cook"), "Project toolbar exposes import plus cook");
    HK_CHECK_MSG(Contains(source, "Recook Dirty"), "Project toolbar keeps dirty recook as maintenance action");
    HK_CHECK_MSG(Contains(source, "Texture/Image"), "Project toolbar exposes texture/image type filter");
    HK_CHECK_MSG(Contains(source, "Reimport & Recook"), "Project asset context menu recooks through cooked selection");
    HK_CHECK_MSG(Contains(source, "Reveal Source"), "Project asset context menu can reveal source");
    HK_CHECK_MSG(Contains(source, "Reveal Cooked"), "Project asset context menu can reveal cooked output");
    HK_CHECK_MSG(Contains(source, "m_Browser.SectionEntries") && Contains(source, "AssetDatabase"),
                 "Project panel renders from cooked ProjectBrowser entries");
    HK_CHECK_MSG(!Contains(source, "##project-details"), "Project panel no longer owns asset details/editor pane");
    HK_CHECK_MSG(Contains(source, "context.SelectAsset") && Contains(source, "requestedPanelFocus"),
                 "Project asset clicks hand selection to Inspector");
    HK_CHECK_MSG(Contains(source, "BeginCookedDragSource(context, entry)") &&
                     source.find("BeginCookedDragSource(context, entry)") < source.find("ImGui::TextWrapped"),
                 "Project panel attaches drag source before tile label text");
    HK_CHECK_MSG(Contains(source, "CookAllDirty()"), "Project panel import/edit flows cook dirty assets");
    HK_CHECK_MSG(CountOccurrences(source, "EditorTooltip::ForLastItem") >= 17,
                 "New Project panel controls expose hover text");
}
