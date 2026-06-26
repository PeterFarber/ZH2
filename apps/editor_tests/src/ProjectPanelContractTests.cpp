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

    HK_CHECK_MSG(Contains(header, "enum class ProjectSection"), "Project panel tracks fixed raw asset sections");
    HK_CHECK_MSG(Contains(header, "m_SelectedAssetId"), "Project panel can select imported raw assets by asset id");
    HK_CHECK_MSG(Contains(header, "m_IconSize"), "Project panel stores icon/list view size");
    HK_CHECK_MSG(Contains(header, "m_TypeFilter"), "Project panel stores a cooked asset type filter");
    HK_CHECK_MSG(Contains(header, "DrawProjectTileButton"), "Project panel centralizes tile button rendering");
    HK_CHECK_MSG(Contains(header, "ThumbnailTextureId"), "Project panel resolves asset thumbnails for tile mode");
    HK_CHECK_MSG(Contains(header, "enum class ContextTargetKind"), "Project panel has explicit right-click targets");
    HK_CHECK_MSG(Contains(header, "struct ContextTarget"), "Project panel stores the right-click target");
    HK_CHECK_MSG(Contains(header, "DrawContentBackgroundContextMenu"),
                 "Project panel declares empty-pane right-click handling");
    HK_CHECK_MSG(Contains(header, "DeleteProjectPath"), "Project panel routes destructive delete through a helper");
    HK_CHECK_MSG(Contains(header, "AssetIdsUnderPath"), "Project panel can find tracked assets under a raw path");
    const std::string previewHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/Project/EditorAssetPreviewRenderer.hpp");
    const std::string previewSource = ReadProjectFile("libs/editor/src/Project/EditorAssetPreviewRenderer.cpp");

    HK_CHECK_MSG(Contains(previewHeader, "class EditorAssetPreviewRenderer"),
                 "Shared editor asset preview renderer type exists");
    HK_CHECK_MSG(Contains(previewHeader, "MaterialPreviewTextureId"),
                 "Shared preview renderer exposes material preview texture ids");
    HK_CHECK_MSG(Contains(previewHeader, "m_MaterialPreviewTargets"),
                 "Shared preview renderer owns reusable material preview targets");
    HK_CHECK_MSG(!Contains(header, "m_MaterialPreviewTargets"),
                 "Project panel no longer owns material preview render targets");

    HK_CHECK_MSG(Contains(source, "DrawSectionList"), "Project panel has a dedicated left raw section list");
    HK_CHECK_MSG(Contains(source, "SectionListWidth"), "Project panel computes section list width from its labels");
    HK_CHECK_MSG(Contains(source, "ImGui::CalcTextSize"), "Project section width measures actual rendered label text");
    HK_CHECK_MSG(!Contains(source, "* 0.32f"), "Project section width is fixed to content instead of viewport percentage");
    HK_CHECK_MSG(Contains(source, "DrawFolderContents"), "Project panel has a dedicated right raw contents pane");
    HK_CHECK_MSG(Contains(source, "DrawStatusStrip"), "Project panel has a bottom path/status strip");
    HK_CHECK_MSG(Contains(source, "DrawViewSizeSlider"), "Project panel has a bottom-right icon size slider");
    HK_CHECK_MSG(Contains(source, "All") && Contains(source, "Models") && Contains(source, "Prefabs") &&
                     Contains(source, "Scenes") && Contains(source, "Textures"),
                 "Project panel exposes default raw asset sections");
    HK_CHECK_MSG(Contains(source, "Import All & Cook"), "Project toolbar exposes import plus cook");
    HK_CHECK_MSG(Contains(source, "Recook Dirty"), "Project toolbar keeps dirty recook as maintenance action");
    HK_CHECK_MSG(Contains(source, "Texture/Image"), "Project toolbar exposes texture/image type filter");
    HK_CHECK_MSG(Contains(source, "Reimport & Recook"), "Project asset context menu recooks through raw asset selection");
    HK_CHECK_MSG(Contains(source, "Reveal Source"), "Project asset context menu can reveal source");
    HK_CHECK_MSG(Contains(source, "Reveal Cooked"), "Project asset context menu can reveal cooked output");
    HK_CHECK_MSG(Contains(source, "m_Browser.Entries(m_SelectedFolder)"),
                 "Project panel renders immediate raw filesystem entries from the selected section folder");
    HK_CHECK_MSG(Contains(source, "std::filesystem::recursive_directory_iterator"),
                 "Project panel searches raw files and subfolders under the selected section");
    HK_CHECK_MSG(Contains(source, "AssetPath::IsMetadataSidecar"),
                 "Project panel hides asset metadata sidecars from visible raw entries and search");
    HK_CHECK_MSG(Contains(source, "entry.type.supported") && Contains(source, "ImportAndCookRawAsset"),
                 "Project panel can import supported raw files before asset selection or drag");
    HK_CHECK_MSG(Contains(source, "DrawRawEntryContextActions") && Contains(source, "Reveal Source") &&
                     Contains(source, "Rename...") && Contains(source, "Delete..."),
                 "Unsupported raw files still keep source file context actions");
    HK_CHECK_MSG(Contains(source, "OpenContextMenuForEntry"), "Project panel opens context menus from item bounds");
    HK_CHECK_MSG(Contains(source, "OpenContextMenuForFolder"),
                 "Project panel opens context menus from folder/background targets");
    HK_CHECK_MSG(Contains(source, "CanRenameDeletePath"), "Project panel guards rename/delete targets");
    HK_CHECK_MSG(Contains(source, "ImGui::IsMouseHoveringRect(min, max)") &&
                     Contains(source, "ImGui::IsMouseClicked(ImGuiMouseButton_Right)"),
                 "Project entries open context menus from full visual bounds");
    HK_CHECK_MSG(!Contains(source, "BeginPopupContextItem(\"##project-entry-context\")"),
                 "Project panel no longer relies on item-local context popups");
    HK_CHECK_MSG(Contains(source, "DrawContentBackgroundContextMenu"), "Project panel handles empty-pane right clicks");
    HK_CHECK_MSG(Contains(source, "BeginPopupContextWindow(\"##project-content-context\"") ||
                     Contains(source, "ImGui::IsWindowHovered"),
                 "Project panel opens a context menu from the contents background");
    HK_CHECK_MSG(Contains(source, "ImGuiPopupFlags_NoOpenOverItems") ||
                     Contains(source, "!ImGui::IsAnyItemHovered()"),
                 "Project background menu does not steal item right-clicks");
    HK_CHECK_MSG(Contains(source, "DrawProjectContextMenu"), "Project panel renders one shared context menu");
    HK_CHECK_MSG(Contains(source, "DrawFolderContextActions"), "Project panel has reusable folder context actions");
    HK_CHECK_MSG(Contains(source, "DrawRawEntryContextActions"), "Project panel has reusable file/asset context actions");
    HK_CHECK_MSG(Contains(source, "Project roots cannot be renamed or deleted"),
                 "Project root rename/delete is disabled");
    HK_CHECK_MSG(Contains(source, "DeleteAssetFiles"), "Project panel deletes cooked outputs for tracked raw assets");
    HK_CHECK_MSG(Contains(source, "InvalidateAssetEvents(context)"),
                 "Project panel invalidates renderer asset caches after delete");
    HK_CHECK_MSG(!Contains(source, "##project-details"), "Project panel no longer owns asset details/editor pane");
    HK_CHECK_MSG(Contains(source, "context.SelectAsset") && Contains(source, "requestedPanelFocus"),
                 "Project asset clicks hand selection to Inspector");
    HK_CHECK_MSG(Contains(source, "context.imguiBridge->TextureIdForAsset(meta->id.Value())"),
                 "Project texture tiles use cooked texture thumbnails");
    HK_CHECK_MSG(Contains(source, "AssetType::Material") &&
                     Contains(source, "m_AssetPreviewRenderer.MaterialPreviewTextureId"),
                 "Project material tiles request rendered material previews from the shared helper");
    HK_CHECK_MSG(!Contains(source, "RenderSceneToTarget") && !Contains(source, "BuiltIn.Sphere"),
                 "Project panel does not own material preview scene rendering");
    HK_CHECK_MSG(Contains(previewSource, "RenderSceneToTarget") &&
                     Contains(previewSource, "sphere_mesh.mesh.yaml") &&
                     Contains(previewSource, "mesh.meshAsset = sphereMesh->id.Value()") &&
                     Contains(previewSource, "materialAsset = materialAssetId") &&
                     Contains(previewSource, "ViewportTextureId"),
                 "Shared preview renderer renders material assets on a sphere into an ImGui texture");
    HK_CHECK_MSG(Contains(source, "BeginRawDragSource(context, entry)") &&
                     source.find("BeginRawDragSource(context, entry)") < source.find("ImGui::TextWrapped"),
                 "Project panel attaches drag source before tile label text");
    HK_CHECK_MSG(Contains(source, "CookAllDirty()"), "Project panel import/edit flows cook dirty assets");
    HK_CHECK_MSG(CountOccurrences(source, "EditorTooltip::ForLastItem") >= 17,
                 "New Project panel controls expose hover text");
}
