#include "Hockey/Editor/Panels/ProjectPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/FileDialog.hpp"
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/ClientFlowSerializer.hpp"

namespace Hockey {

namespace {

constexpr int kMaxSearchResults = 400;
constexpr float kListModeThreshold = 32.0f;
constexpr std::uint32_t kMaterialPreviewSize = 128;

struct SectionInfo {
    ProjectSection section;
    AssetType type;
    const char* label;
};

constexpr SectionInfo kSections[] = {
    {ProjectSection::All, AssetType::Unknown, "All"},
    {ProjectSection::Models, AssetType::Model, "Models"},
    {ProjectSection::Meshes, AssetType::Mesh, "Meshes"},
    {ProjectSection::Materials, AssetType::Material, "Materials"},
    {ProjectSection::Prefabs, AssetType::Prefab, "Prefabs"},
    {ProjectSection::Scenes, AssetType::Scene, "Scenes"},
    {ProjectSection::Textures, AssetType::Texture, "Textures"},
    {ProjectSection::Shaders, AssetType::Shader, "Shaders"},
};

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool SamePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    return ToLower(lhs.lexically_normal().generic_string()) == ToLower(rhs.lexically_normal().generic_string());
}

std::filesystem::path ResolveProjectPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path.lexically_normal();
    }
    return (Paths::Get().root / path).lexically_normal();
}

bool PathIsWithin(const std::filesystem::path& path, const std::filesystem::path& root) {
    const std::string value = ToLower(ResolveProjectPath(path).generic_string());
    std::string prefix = ToLower(ResolveProjectPath(root).generic_string());
    if (value == prefix) {
        return true;
    }
    if (!prefix.empty() && prefix.back() != '/') {
        prefix.push_back('/');
    }
    return !prefix.empty() && value.rfind(prefix, 0) == 0;
}

std::filesystem::path ProjectRelativePath(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    std::error_code ec;
    const std::filesystem::path relative = std::filesystem::relative(ResolveProjectPath(path), Paths::Get().root, ec);
    return ec ? path.lexically_normal() : relative.lexically_normal();
}

bool IsInspectableAsset(const AssetMetadata& meta) {
    return meta.id.IsValid() && !meta.missing && !meta.rawPath.empty();
}

const AssetMetadata* FindAssetMeta(EditorContext& context, const std::filesystem::path& absolute) {
    if (context.assetManager == nullptr) {
        return nullptr;
    }
    return context.assetManager->Database().FindByRawPath(ProjectRelativePath(absolute));
}

const char* AssetStatusLabel(const AssetMetadata& meta) {
    if (meta.missing) {
        return "missing";
    }
    if (meta.dirty) {
        return "dirty";
    }
    if (meta.cooked) {
        return "cooked";
    }
    return "imported";
}

ImVec4 ColorForType(EditorFileType type, bool supported) {
    switch (type) {
    case EditorFileType::Folder:
        return ImVec4(0.86f, 0.78f, 0.46f, 1.0f);
    case EditorFileType::Scene:
        return ImVec4(0.55f, 0.80f, 1.0f, 1.0f);
    case EditorFileType::Prefab:
        return ImVec4(0.70f, 0.85f, 0.55f, 1.0f);
    case EditorFileType::Material:
        return ImVec4(0.92f, 0.66f, 0.85f, 1.0f);
    case EditorFileType::UIScreen:
        return ImVec4(0.56f, 0.88f, 0.82f, 1.0f);
    case EditorFileType::UITheme:
        return ImVec4(0.86f, 0.72f, 0.54f, 1.0f);
    case EditorFileType::ClientFlow:
        return ImVec4(0.78f, 0.86f, 0.58f, 1.0f);
    case EditorFileType::Image:
        return ImVec4(0.66f, 0.78f, 0.96f, 1.0f);
    case EditorFileType::Model:
        return ImVec4(0.78f, 0.70f, 0.95f, 1.0f);
    default:
        return supported ? ImVec4(0.85f, 0.85f, 0.85f, 1.0f) : ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    }
}

std::filesystem::path UniqueDestination(const std::filesystem::path& dir, const std::string& fileName) {
    std::filesystem::path candidate = dir / fileName;
    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec)) {
        return candidate;
    }

    const std::size_t dot = fileName.find('.');
    const std::string stem = dot == std::string::npos ? fileName : fileName.substr(0, dot);
    const std::string exts = dot == std::string::npos ? std::string{} : fileName.substr(dot);
    for (int n = 1; n < 10000; ++n) {
        candidate = dir / (stem + "_" + std::to_string(n) + exts);
        if (!std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return candidate;
}

ProjectSection SectionForAssetType(AssetType type) {
    switch (type) {
    case AssetType::Model:
        return ProjectSection::Models;
    case AssetType::Mesh:
        return ProjectSection::Meshes;
    case AssetType::Material:
        return ProjectSection::Materials;
    case AssetType::Prefab:
        return ProjectSection::Prefabs;
    case AssetType::Scene:
        return ProjectSection::Scenes;
    case AssetType::Texture:
        return ProjectSection::Textures;
    case AssetType::Shader:
        return ProjectSection::Shaders;
    default:
        return ProjectSection::All;
    }
}

const char* SectionLabel(ProjectSection section) {
    for (const SectionInfo& info : kSections) {
        if (info.section == section) {
            return info.label;
        }
    }
    return "All";
}

AssetType SectionType(ProjectSection section) {
    for (const SectionInfo& info : kSections) {
        if (info.section == section) {
            return info.type;
        }
    }
    return AssetType::Unknown;
}

std::string SectionDisplayLabel(const SectionInfo& info,
                                const ProjectBrowser& browser,
                                const std::filesystem::path& sectionRoot) {
    std::string label = info.label;
    std::error_code ec;
    if (std::filesystem::is_directory(sectionRoot, ec)) {
        label += " (" + std::to_string(browser.Entries(sectionRoot).size()) + ")";
    }
    return label;
}

std::filesystem::path RawFolderForType(EditorContext& context, AssetType type) {
    const std::filesystem::path rawRoot = context.assetManager != nullptr ? context.assetManager->Info().rawRoot
                                                                          : Paths::Get().rawAssets;
    if (type == AssetType::Unknown) {
        return rawRoot;
    }
    return rawRoot / AssetPath::CookedSubdirectory(type);
}

bool TypeMatches(EditorFileType filter, const ProjectEntry& entry) {
    if (filter == EditorFileType::Unknown) {
        return true;
    }
    if (filter == EditorFileType::Folder) {
        return entry.isDirectory;
    }
    if (entry.isDirectory) {
        return false;
    }
    if (filter == EditorFileType::ShaderSource) {
        return entry.type.type == EditorFileType::ShaderSource || entry.type.type == EditorFileType::ShaderBinary;
    }
    return entry.type.type == filter;
}

bool IsCookableRawType(EditorFileType type) {
    switch (type) {
    case EditorFileType::Scene:
    case EditorFileType::Prefab:
    case EditorFileType::Material:
    case EditorFileType::ShaderSource:
    case EditorFileType::Image:
    case EditorFileType::Model:
        return true;
    default:
        return false;
    }
}

} // namespace

ProjectPanel::ProjectPanel() : Panel(EditorPanelNames::kProject) {}

void ProjectPanel::RequestModal(Modal modal, const std::filesystem::path& target) {
    m_Modal = modal;
    m_ModalTarget = target;
    m_OpenModalRequested = true;
    if (modal == Modal::Rename) {
        std::snprintf(m_NameBuffer, sizeof(m_NameBuffer), "%s", target.filename().string().c_str());
    } else {
        m_NameBuffer[0] = '\0';
    }
}

void ProjectPanel::OnImGui(EditorContext& context) {
    if (BeginWindow()) {
        EnsureSelectionState(context);

        const bool projectFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGuiIO& io = ImGui::GetIO();
        const bool canShortcut = projectFocused && !io.WantTextInput && !ImGui::IsAnyItemActive();
        if (canShortcut && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false)) {
            m_FocusSearchNextFrame = true;
        }
        if (canShortcut && ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
            m_SelectedFolder = m_Browser.ParentFolderWithinRoots(m_SelectedFolder);
            m_SelectedAssetId = {};
        }
        if (canShortcut && ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            if (const AssetMetadata* meta = SelectedMetadata(context)) {
                RequestModal(Modal::Rename, ResolveProjectPath(meta->rawPath));
            } else if (!m_SelectedFolder.empty()) {
                RequestModal(Modal::Rename, m_SelectedFolder);
            }
        }
        if (canShortcut && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            if (const AssetMetadata* meta = SelectedMetadata(context)) {
                RequestModal(Modal::ConfirmDelete, ResolveProjectPath(meta->rawPath));
            } else if (!m_SelectedFolder.empty()) {
                RequestModal(Modal::ConfirmDelete, m_SelectedFolder);
            }
        }
        if (canShortcut && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            if (const AssetMetadata* meta = SelectedMetadata(context)) {
                const std::filesystem::path raw = ResolveProjectPath(meta->rawPath);
                if (meta->type == AssetType::Scene) {
                    OpenSceneFile(context, raw);
                } else if (meta->type == AssetType::Prefab) {
                    InstantiatePrefabFile(context, raw);
                }
            }
        }

        DrawToolbar(context);
        ImGui::Separator();

        const float statusHeight = ImGui::GetFrameHeightWithSpacing() + 4.0f;
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float bodyHeight = std::max(0.0f, avail.y - statusHeight);

        if (ImGui::BeginChild("##project-body", ImVec2(0.0f, bodyHeight), false)) {
            if (ImGui::BeginChild("##project-section-list", ImVec2(SectionListWidth(context), 0.0f), true)) {
                DrawSectionList(context);
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("##project-right", ImVec2(0.0f, 0.0f), false)) {
                if (ImGui::BeginChild("##project-contents", ImVec2(0.0f, 0.0f), true)) {
                    if (m_Browser.SearchActive()) {
                        DrawSearchResults(context);
                    } else {
                        DrawFolderContents(context);
                    }
                    DrawContentBackgroundContextMenu(context);
                    DrawProjectContextMenu(context);
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        DrawStatusStrip(context);
        DrawModals(context);
    }
    EndWindow();
}

void ProjectPanel::EnsureSelectionState(EditorContext& context) {
    context.SyncAssetSelectionWithEntitySelection();
    if (context.SelectedAsset().IsValid()) {
        m_SelectedAssetId = context.SelectedAsset();
    }

    if (const AssetMetadata* meta = SelectedMetadata(context)) {
        m_SelectedSection = SectionForAssetType(meta->type);
        m_SelectedFolder = ResolveProjectPath(meta->rawPath).parent_path();
        m_Browser.Select(ResolveProjectPath(meta->rawPath));
    } else {
        if (!context.SelectedAsset().IsValid()) {
            m_SelectedAssetId = {};
        }
        const std::filesystem::path sectionRoot = SelectedSectionRawFolder(context, AssetType::Unknown);
        if (m_SelectedFolder.empty() || !PathIsWithin(m_SelectedFolder, sectionRoot)) {
            m_SelectedFolder = sectionRoot;
        }
    }
}

void ProjectPanel::DrawToolbar(EditorContext& context) {
    if (ImGui::Button("Create")) {
        ImGui::OpenPopup("##project-create");
    }
    EditorTooltip::ForLastItem("Create source assets under the selected Project folder");
    if (ImGui::BeginPopup("##project-create")) {
        if (ImGui::MenuItem("New Folder...")) {
            RequestModal(Modal::CreateFolder, SelectedSectionRawFolder(context, AssetType::Unknown));
        }
        EditorTooltip::ForLastItem("Create a source folder under the selected Project section");
        ImGui::BeginDisabled(context.assetManager == nullptr);
        if (ImGui::MenuItem("New Material")) {
            CreateMaterialAsset(context, SelectedSectionRawFolder(context, AssetType::Material));
        }
        EditorTooltip::ForLastItem("Create, import, and cook a new material source asset");
        ImGui::EndDisabled();
        if (ImGui::MenuItem("New UI Screen")) {
            CreateUIScreenAsset(context, SelectedSectionRawFolder(context, AssetType::Unknown));
        }
        EditorTooltip::ForLastItem("Create a new RmlUi screen document");
        if (ImGui::MenuItem("New UI Theme")) {
            CreateUIThemeAsset(context, SelectedSectionRawFolder(context, AssetType::Unknown));
        }
        EditorTooltip::ForLastItem("Create a new RmlUi stylesheet");
        if (ImGui::MenuItem("New Client Flow")) {
            CreateClientFlowAsset(context, SelectedSectionRawFolder(context, AssetType::Unknown));
        }
        EditorTooltip::ForLastItem("Create a new runtime client-flow asset");
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(context.assetManager == nullptr);
    if (ImGui::Button("Import...")) {
        ImportSelectedFile(context);
    }
    EditorTooltip::ForLastItem("Import a source file into the selected raw folder, then cook it");
    ImGui::SameLine();
    if (ImGui::Button("Import All & Cook")) {
        ImportAllAndCook(context);
    }
    EditorTooltip::ForLastItem("Discover, import, cook dirty assets, and save the asset database");
    ImGui::SameLine();
    if (ImGui::Button("Recook Dirty")) {
        RecookDirty(context);
    }
    EditorTooltip::ForLastItem("Maintenance action: cook assets already marked dirty");
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (m_FocusSearchNextFrame) {
        ImGui::SetKeyboardFocusHere();
        m_FocusSearchNextFrame = false;
    }
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##project-search", "Search raw assets...", m_Browser.SearchBuffer(),
                             m_Browser.SearchBufferSize());
    EditorTooltip::ForLastItem("Search raw folders and source assets by name or path");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        m_Browser.SearchBuffer()[0] = '\0';
    }
    EditorTooltip::ForLastItem("Clear the Project search filter");

    struct TypeOption {
        EditorFileType type;
        const char* label;
    };
    static constexpr TypeOption kTypes[] = {
        {EditorFileType::Unknown, "All"},
        {EditorFileType::Scene, "Scene"},
        {EditorFileType::Prefab, "Prefab"},
        {EditorFileType::Material, "Material"},
        {EditorFileType::UIScreen, "UI Screen"},
        {EditorFileType::UITheme, "UI Theme"},
        {EditorFileType::ClientFlow, "Client Flow"},
        {EditorFileType::Image, "Texture/Image"},
        {EditorFileType::Model, "Model"},
        {EditorFileType::Model, "Mesh"},
        {EditorFileType::ShaderSource, "Shader"},
    };
    int currentType = 0;
    for (int i = 0; i < static_cast<int>(std::size(kTypes)); ++i) {
        if (kTypes[i].type == m_TypeFilter) {
            currentType = i;
            break;
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::BeginCombo("##project-type-filter", kTypes[currentType].label)) {
        for (int i = 0; i < static_cast<int>(std::size(kTypes)); ++i) {
            const bool selected = i == currentType;
            if (ImGui::Selectable(kTypes[i].label, selected)) {
                m_TypeFilter = kTypes[i].type;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    EditorTooltip::ForLastItem("Filter visible raw entries by asset type");

    if (!m_Status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "%s", m_Status.c_str());
    }
}

float ProjectPanel::SectionListWidth(EditorContext& context) const {
    float maxLabelWidth = 0.0f;
    for (const SectionInfo& info : kSections) {
        const std::string label = SectionDisplayLabel(info, m_Browser, RawFolderForType(context, info.type));
        maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(label.c_str()).x);
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    return std::ceil(maxLabelWidth + (style.WindowPadding.x * 2.0f) + (style.FramePadding.x * 2.0f) +
                     (style.ChildBorderSize * 2.0f));
}

void ProjectPanel::DrawSectionList(EditorContext& context) {
    for (const SectionInfo& info : kSections) {
        const bool selected = m_SelectedSection == info.section;
        const std::string label = SectionDisplayLabel(info, m_Browser, RawFolderForType(context, info.type));

        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
            m_SelectedSection = info.section;
            m_SelectedAssetId = {};
            context.ClearAssetSelection();
            m_SelectedFolder = SelectedSectionRawFolder(context, info.type);
        }
        EditorTooltip::ForLastItem("Show raw source assets in this Project section");
    }
}

void ProjectPanel::DrawFolderContents(EditorContext& context) {
    std::vector<ProjectEntry> entries;
    for (const ProjectEntry& entry : m_Browser.Entries(m_SelectedFolder)) {
        if (MatchesSearchAndFilter(entry, true)) {
            entries.push_back(entry);
        }
    }
    if (entries.empty()) {
        ImGui::TextDisabled("No raw assets in %s match the current filter.", SectionLabel(m_SelectedSection));
        return;
    }

    if (m_IconSize <= kListModeThreshold) {
        for (const ProjectEntry& entry : entries) {
            DrawRawEntry(context, entry);
        }
        return;
    }

    const float cellWidth = std::max(96.0f, m_IconSize + 34.0f);
    const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));
    if (ImGui::BeginTable("##project-tile-grid", columns, ImGuiTableFlags_SizingFixedFit)) {
        for (const ProjectEntry& entry : entries) {
            ImGui::TableNextColumn();
            DrawRawEntry(context, entry);
        }
        ImGui::EndTable();
    }
}

void ProjectPanel::DrawSearchResults(EditorContext& context) {
    bool truncated = false;
    const std::vector<ProjectEntry> entries = SearchEntries(context, &truncated);
    if (entries.empty()) {
        ImGui::TextDisabled("No raw search results.");
        return;
    }

    for (const ProjectEntry& entry : entries) {
        DrawRawEntry(context, entry);
    }
    if (truncated) {
        ImGui::TextDisabled("(results truncated)");
    }
}

void ProjectPanel::DrawRawEntry(EditorContext& context, const ProjectEntry& entry) {
    ImGui::PushID(entry.path.string().c_str());
    const AssetMetadata* meta = MetadataForRawEntry(context, entry);
    const bool selected =
        entry.isDirectory ? SamePath(entry.path, m_SelectedFolder)
                          : ((meta != nullptr && meta->id == m_SelectedAssetId) ||
                             SamePath(entry.path, m_Browser.Selected()));
    const ImVec4 color = ColorForType(entry.type.type, entry.type.supported);
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, color);

    if (m_IconSize <= kListModeThreshold) {
        const std::string label = EditorIconLabel(EditorIconForAssetType(entry.type.type), entry.name);
        if (ImGui::Selectable(label.c_str(),
                              selected,
                              ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            SelectRawEntry(context, entry);
        }
        BeginRawDragSource(context, entry);
        HandleRawActivation(context, entry);
    } else {
        const ImVec2 size(m_IconSize, m_IconSize);
        if (DrawProjectTileButton(context, entry, meta, size)) {
            SelectRawEntry(context, entry);
        }
        if (selected) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                ImGui::GetColorU32(ImGuiCol_Text));
        }
        BeginRawDragSource(context, entry);
        HandleRawActivation(context, entry);
        ImGui::TextWrapped("%s", entry.name.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    const ImVec2 groupMin = ImGui::GetItemRectMin();
    const ImVec2 groupMax = ImGui::GetItemRectMax();
    OpenContextMenuForEntry(context, entry, groupMin, groupMax);
    ImGui::PopID();
}

void ProjectPanel::OpenContextMenuForEntry(EditorContext& context,
                                           const ProjectEntry& entry,
                                           const ImVec2& min,
                                           const ImVec2& max) {
    if (!ImGui::IsMouseHoveringRect(min, max) || !ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        return;
    }

    m_ContextTarget = {};
    m_ContextTarget.kind = entry.isDirectory ? ContextTargetKind::Folder : ContextTargetKind::RawEntry;
    m_ContextTarget.entry = entry;
    m_ContextTarget.folder = entry.isDirectory ? entry.path : entry.path.parent_path();
    SelectContextTarget(context);
    ImGui::OpenPopup("##project-context-menu");
}

void ProjectPanel::OpenContextMenuForFolder(EditorContext& context,
                                            const std::filesystem::path& folder) {
    m_ContextTarget = {};
    m_ContextTarget.kind = ContextTargetKind::EmptyArea;
    m_ContextTarget.folder = folder;
    SelectContextTarget(context);
    ImGui::OpenPopup("##project-context-menu");
}

bool ProjectPanel::CanRenameDeletePath(const std::filesystem::path& path) const {
    if (path.empty()) {
        return false;
    }
    if (!m_Browser.IsWithinRoots(path)) {
        return false;
    }
    return !SamePath(m_Browser.RootForPath(path), path);
}

void ProjectPanel::SelectContextTarget(EditorContext& context) {
    if (m_ContextTarget.kind == ContextTargetKind::Folder) {
        m_SelectedFolder = m_ContextTarget.entry.path;
        m_SelectedAssetId = {};
        m_Browser.Select(m_ContextTarget.entry.path);
        context.ClearAssetSelection();
        return;
    }

    if (m_ContextTarget.kind == ContextTargetKind::RawEntry) {
        const ProjectEntry& entry = m_ContextTarget.entry;
        m_Browser.Select(entry.path);
        if (const AssetMetadata* meta = MetadataForRawEntry(context, entry)) {
            m_SelectedAssetId = meta->id;
            context.SelectAsset(meta->id);
            context.requestedPanelFocus = EditorPanelNames::kInspector;
        } else {
            m_SelectedAssetId = {};
            context.ClearAssetSelection();
        }
        return;
    }

    if (m_ContextTarget.kind == ContextTargetKind::EmptyArea && !m_ContextTarget.folder.empty()) {
        m_SelectedFolder = m_ContextTarget.folder;
        m_SelectedAssetId = {};
        context.ClearAssetSelection();
    }
}

bool ProjectPanel::DrawProjectTileButton(EditorContext& context,
                                         const ProjectEntry& entry,
                                         const AssetMetadata* meta,
                                         const ImVec2& size) {
    const std::uint64_t thumbnail = ThumbnailTextureId(context, entry, meta);
    if (thumbnail == 0) {
        const EditorIcon icon = EditorIconForAssetType(entry.type.type);
        const char* glyph = EditorIconGlyph(icon);
        const char* tileLabel = (glyph != nullptr && glyph[0] != '\0') ? glyph : "Asset";
        return ImGui::Button(tileLabel, size);
    }

    const bool pressed = ImGui::InvisibleButton("##project-tile-button", size);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImU32 bg = ImGui::GetColorU32(ImGui::IsItemActive()   ? ImGuiCol_ButtonActive
                                        : ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered
                                                                 : ImGuiCol_Button);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, bg, style.FrameRounding);
    drawList->AddImage(static_cast<ImTextureID>(thumbnail), min, max);
    drawList->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding);
    return pressed;
}

std::uint64_t ProjectPanel::ThumbnailTextureId(EditorContext& context,
                                               const ProjectEntry& entry,
                                               const AssetMetadata* meta) {
    (void)entry;
    if (meta == nullptr || context.imguiBridge == nullptr) {
        return 0;
    }
    if (meta->type == AssetType::Texture) {
        return context.imguiBridge->TextureIdForAsset(meta->id.Value());
    }
    if (meta->type == AssetType::Material) {
        return m_AssetPreviewRenderer.MaterialPreviewTextureId(context, meta->id.Value(), kMaterialPreviewSize);
    }
    return 0;
}

void ProjectPanel::DrawContentBackgroundContextMenu(EditorContext& context) {
    const std::filesystem::path folder =
        m_SelectedFolder.empty() ? SelectedSectionRawFolder(context, SelectedSectionAssetType())
                                 : m_SelectedFolder;

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        OpenContextMenuForFolder(context, folder);
    }
}

void ProjectPanel::DrawProjectContextMenu(EditorContext& context) {
    if (!ImGui::BeginPopup("##project-context-menu")) {
        return;
    }

    switch (m_ContextTarget.kind) {
    case ContextTargetKind::Folder:
        DrawFolderContextActions(context, m_ContextTarget.entry.path, true);
        break;
    case ContextTargetKind::RawEntry:
        DrawRawEntryContextActions(context, m_ContextTarget.entry);
        break;
    case ContextTargetKind::EmptyArea:
        DrawFolderContextActions(context, m_ContextTarget.folder, true);
        break;
    case ContextTargetKind::None:
        ImGui::TextDisabled("No Project item selected.");
        break;
    }

    ImGui::EndPopup();
}

void ProjectPanel::DrawFolderContextActions(EditorContext& context,
                                            const std::filesystem::path& folder,
                                            bool includeFolderRenameDelete) {
    if (ImGui::MenuItem("New Folder...")) {
        RequestModal(Modal::CreateFolder, folder);
    }
    if (context.assetManager != nullptr && ImGui::MenuItem("New Material")) {
        CreateMaterialAsset(context, folder);
    }
    if (ImGui::MenuItem("New UI Screen")) {
        CreateUIScreenAsset(context, folder);
    }
    if (ImGui::MenuItem("New UI Theme")) {
        CreateUIThemeAsset(context, folder);
    }
    if (ImGui::MenuItem("New Client Flow")) {
        CreateClientFlowAsset(context, folder);
    }

    if (m_ContextTarget.kind == ContextTargetKind::EmptyArea) {
        ImGui::Separator();
        if (context.assetManager != nullptr && ImGui::MenuItem("Import...")) {
            ImportSelectedFile(context);
        }
        if (context.assetManager != nullptr && ImGui::MenuItem("Import All & Cook")) {
            ImportAllAndCook(context);
        }
        if (context.assetManager != nullptr && ImGui::MenuItem("Recook Dirty")) {
            RecookDirty(context);
        }
    }

    ImGui::Separator();
    if (includeFolderRenameDelete && CanRenameDeletePath(folder)) {
        if (ImGui::MenuItem("Rename...")) {
            RequestModal(Modal::Rename, folder);
        }
        if (ImGui::MenuItem("Delete...")) {
            RequestModal(Modal::ConfirmDelete, folder);
        }
    } else if (includeFolderRenameDelete) {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Rename...");
        ImGui::MenuItem("Delete...");
        ImGui::EndDisabled();
        EditorTooltip::ForLastItem("Project roots cannot be renamed or deleted from this menu.");
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Reveal Source")) {
        ProjectBrowser::Reveal(folder);
    }
}

void ProjectPanel::DrawRawEntryContextActions(EditorContext& context, const ProjectEntry& entry) {
    const AssetMetadata* meta = MetadataForRawEntry(context, entry);
    if (entry.type.type == EditorFileType::Scene && ImGui::MenuItem("Open Scene")) {
        OpenSceneFile(context, entry.path);
    }
    if (entry.type.type == EditorFileType::Prefab && ImGui::MenuItem("Instantiate Prefab")) {
        InstantiatePrefabFile(context, entry.path);
    }
    if (entry.type.type == EditorFileType::ClientFlow && ImGui::MenuItem("Open Client Flow")) {
        OpenClientFlowFile(context, entry.path);
    }
    if (entry.type.type == EditorFileType::Scene || entry.type.type == EditorFileType::Prefab ||
        entry.type.type == EditorFileType::ClientFlow) {
        ImGui::Separator();
    }
    if (context.assetManager != nullptr && meta == nullptr && IsCookableRawType(entry.type.type) &&
        ImGui::MenuItem("Import & Cook")) {
        ImportAndCookRawAsset(context, entry.path);
    }
    EditorTooltip::ForLastItem("Import this supported raw source asset and cook dirty dependents");
    if (context.assetManager != nullptr && meta != nullptr && ImGui::MenuItem("Reimport & Recook")) {
        ReimportAndRecook(context, *meta);
    }
    EditorTooltip::ForLastItem("Reimport the raw source asset, recook dirty dependents, and save the database");
    if (context.assetManager != nullptr && meta != nullptr && ImGui::MenuItem("Recook")) {
        Recook(context, *meta);
    }
    EditorTooltip::ForLastItem("Mark this asset dirty and cook it immediately");
    if (context.assetManager != nullptr && ImGui::MenuItem("Validate References")) {
        std::vector<AssetValidationIssue> issues;
        const Status status = context.assetManager->ValidateReferences(&issues);
        m_Status = status ? "Validation passed (" + std::to_string(issues.size()) + " notes)"
                          : "Validation found issues: " + std::to_string(issues.size());
    }
    EditorTooltip::ForLastItem("Check asset database references for missing or stale IDs");
    if (meta != nullptr && ImGui::MenuItem("Copy Asset ID")) {
        ImGui::SetClipboardText(meta->id.ToString().c_str());
    }
    EditorTooltip::ForLastItem("Copy this asset's stable ID to the clipboard");
    if (ImGui::MenuItem("Reveal Source")) {
        ProjectBrowser::Reveal(entry.path);
    }
    EditorTooltip::ForLastItem("Reveal the raw source file in the system file manager");
    if (meta != nullptr && !meta->cookedPath.empty() && ImGui::MenuItem("Reveal Cooked")) {
        ProjectBrowser::Reveal(ResolveProjectPath(meta->cookedPath));
    }
    EditorTooltip::ForLastItem("Reveal the cooked output file in the system file manager");
    ImGui::Separator();
    if (CanRenameDeletePath(entry.path) && ImGui::MenuItem("Rename...")) {
        RequestModal(Modal::Rename, entry.path);
    }
    if (CanRenameDeletePath(entry.path) && ImGui::MenuItem("Delete...")) {
        RequestModal(Modal::ConfirmDelete, entry.path);
    }
}

void ProjectPanel::DrawStatusStrip(EditorContext& context) {
    const AssetMetadata* meta = SelectedMetadata(context);
    if (meta != nullptr) {
        const std::filesystem::path raw = ResolveProjectPath(meta->rawPath);
        const std::filesystem::path cooked = ResolveProjectPath(meta->cookedPath);
        ImGui::Text("Selected: %s | Raw: %s | Cooked: %s", raw.filename().string().c_str(),
                    ProjectRelativePath(raw).generic_string().c_str(), ProjectRelativePath(cooked).generic_string().c_str());
    } else if (m_Browser.SearchActive()) {
        ImGui::Text("Search in %s: %s", SectionLabel(m_SelectedSection), m_Browser.SearchBuffer());
    } else {
        ImGui::Text("Section: %s", SectionLabel(m_SelectedSection));
    }
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 170.0f);
    DrawViewSizeSlider();
}

void ProjectPanel::DrawViewSizeSlider() {
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("##ProjectViewSize", &m_IconSize, 24.0f, 112.0f, "");
    EditorTooltip::ForLastItem("Project icon size; minimum switches to compact list mode");
}

void ProjectPanel::BeginRawDragSource(EditorContext& context, const ProjectEntry& entry) {
    if (entry.isDirectory) {
        return;
    }
    if (entry.type.type == EditorFileType::Prefab) {
        if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            return;
        }
        const std::string pathString = entry.path.string();
        ImGui::SetDragDropPayload(kPrefabDragDropType, pathString.c_str(), pathString.size() + 1);
        ImGui::TextUnformatted(entry.name.c_str());
        ImGui::EndDragDropSource();
        return;
    }

    if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        return;
    }
    const AssetMetadata* meta = MetadataForRawEntry(context, entry);
    if (meta == nullptr && IsCookableRawType(entry.type.type) && context.assetManager != nullptr) {
        ImportAndCookRawAsset(context, entry.path);
        meta = MetadataForRawEntry(context, entry);
    }
    if (meta == nullptr) {
        ImGui::EndDragDropSource();
        return;
    }
    const AssetType type = meta != nullptr ? meta->type : AssetType::Unknown;
    const AssetDragPayload payload{meta->id.Value(), static_cast<int>(type)};
    ImGui::SetDragDropPayload(kAssetDragDropType, &payload, sizeof(payload));
    ImGui::Text("%s (%s)", entry.name.c_str(), AssetTypeToString(type).c_str());
    ImGui::EndDragDropSource();
}

void ProjectPanel::HandleRawActivation(EditorContext& context, const ProjectEntry& entry) {
    if (!ImGui::IsItemHovered() || !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        return;
    }
    if (entry.isDirectory) {
        m_SelectedFolder = entry.path;
        m_SelectedAssetId = {};
    } else if (entry.type.type == EditorFileType::Scene) {
        OpenSceneFile(context, entry.path);
    } else if (entry.type.type == EditorFileType::Prefab) {
        InstantiatePrefabFile(context, entry.path);
    } else if (entry.type.type == EditorFileType::ClientFlow) {
        OpenClientFlowFile(context, entry.path);
    }
}

void ProjectPanel::SelectRawEntry(EditorContext& context, const ProjectEntry& entry) {
    if (entry.isDirectory) {
        m_SelectedFolder = entry.path;
        m_SelectedAssetId = {};
        m_Browser.Select(entry.path);
        context.ClearAssetSelection();
        return;
    }

    m_Browser.Select(entry.path);
    const AssetMetadata* meta = MetadataForRawEntry(context, entry);
    if (meta == nullptr && IsCookableRawType(entry.type.type) && context.assetManager != nullptr) {
        ImportAndCookRawAsset(context, entry.path);
        return;
    }

    if (meta != nullptr) {
        m_SelectedAssetId = meta->id;
        context.SelectAsset(meta->id);
        context.requestedPanelFocus = EditorPanelNames::kInspector;
    } else {
        m_SelectedAssetId = {};
        context.ClearAssetSelection();
    }
}

void ProjectPanel::ReimportAndRecook(EditorContext& context, const AssetMetadata& meta) {
    if (context.assetManager == nullptr) {
        return;
    }
    const AssetID id = meta.id;
    const std::filesystem::path rawPath = ResolveProjectPath(meta.rawPath);
    if (const Status status = context.assetManager->ImportAsset(rawPath); !status) {
        m_Status = status.error;
        return;
    }
    context.assetManager->Database().MarkDirtyWithDependents(id);
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Reimported & recooked";
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
    if (context.assetManager->Database().Find(id) != nullptr) {
        m_SelectedAssetId = id;
    }
}

void ProjectPanel::Recook(EditorContext& context, const AssetMetadata& meta) {
    if (context.assetManager == nullptr) {
        return;
    }
    context.assetManager->Database().MarkDirty(meta.id);
    if (const Status status = context.assetManager->CookAsset(meta.id); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Recooked " + ResolveProjectPath(meta.rawPath).filename().string();
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
}

void ProjectPanel::ImportSelectedFile(EditorContext& context) {
    const auto chosen = FileDialog::OpenFile(
        {
            {"All importable", "gltf,glb,png,jpg,jpeg,tga,bmp,hdr,ktx,ktx2,yaml,glsl,vert,frag,comp"},
            {"Models", "gltf,glb"},
            {"Textures", "png,jpg,jpeg,tga,bmp,hdr,ktx,ktx2"},
            {"Materials / Scenes / Prefabs", "yaml"},
            {"Shaders", "glsl,vert,frag,comp"},
        },
        m_SelectedFolder);
    if (chosen) {
        ImportExternalAsset(context, *chosen);
    }
}

void ProjectPanel::ImportExternalAsset(EditorContext& context, const std::filesystem::path& source) {
    if (context.assetManager == nullptr) {
        return;
    }
    const AssetType type = AssetManager::ClassifyExtension(source);
    if (type == AssetType::Unknown) {
        m_Status = "Unsupported asset extension";
        return;
    }

    std::filesystem::path targetDir = context.assetManager->Info().rawRoot / AssetPath::CookedSubdirectory(type);
    if (const Status status = FileSystem::CreateDirectories(targetDir); !status) {
        m_Status = status.error;
        return;
    }

    const std::filesystem::path dest = UniqueDestination(targetDir, source.filename().string());
    std::error_code ec;
    std::filesystem::copy_file(source, dest, std::filesystem::copy_options::none, ec);
    if (ec) {
        m_Status = "Import copy failed: " + ec.message();
        return;
    }
    ImportAndCookRawAsset(context, dest);
}

bool ProjectPanel::ImportAndCookRawAsset(EditorContext& context, const std::filesystem::path& rawPath) {
    if (context.assetManager == nullptr) {
        return false;
    }
    if (const Status status = context.assetManager->ImportAsset(rawPath); !status) {
        m_Status = status.error;
        return false;
    }
    const AssetMetadata* imported = FindAssetMeta(context, rawPath);
    if (imported != nullptr) {
        context.assetManager->Database().MarkDirtyWithDependents(imported->id);
    }
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Imported and cooked " + rawPath.filename().string();
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
    if (const AssetMetadata* meta = FindAssetMeta(context, rawPath)) {
        if (IsInspectableAsset(*meta)) {
            m_SelectedAssetId = meta->id;
            context.SelectAsset(meta->id);
            context.requestedPanelFocus = EditorPanelNames::kInspector;
            m_SelectedFolder = ResolveProjectPath(meta->rawPath).parent_path();
            m_SelectedSection = SectionForAssetType(meta->type);
        }
    }
    return true;
}

void ProjectPanel::ImportAllAndCook(EditorContext& context) {
    if (context.assetManager == nullptr) {
        return;
    }
    const Status importStatus = context.assetManager->ImportAll();
    const std::size_t dirtyCount = context.assetManager->Database().DirtyAssets().size();
    const Status cookStatus = context.assetManager->CookAllDirty();
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
    if (!importStatus) {
        m_Status = importStatus.error;
    } else if (!cookStatus) {
        m_Status = "Cook failed after importing: " + cookStatus.error;
    } else {
        m_Status = "Imported all & cooked " + std::to_string(dirtyCount) + " dirty assets";
    }
}

void ProjectPanel::RecookDirty(EditorContext& context) {
    if (context.assetManager == nullptr) {
        return;
    }
    const std::size_t dirtyCount = context.assetManager->Database().DirtyAssets().size();
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Recooked " + std::to_string(dirtyCount) + " dirty assets";
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
}

void ProjectPanel::InvalidateAssetEvents(EditorContext& context) {
    if (context.assetManager == nullptr || context.renderer == nullptr) {
        return;
    }
    const std::vector<AssetEvent>& events = context.assetManager->PollEvents();
    for (const AssetEvent& event : events) {
        context.renderer->InvalidateAsset(event.id.Value());
    }
}

std::vector<ProjectEntry> ProjectPanel::SearchEntries(EditorContext& context, bool* truncated) const {
    std::vector<ProjectEntry> results;
    if (truncated != nullptr) {
        *truncated = false;
    }
    const std::filesystem::path root = SelectedSectionRawFolder(context, AssetType::Unknown);
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        return results;
    }

    for (const auto& dirEntry :
         std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            break;
        }
        ProjectEntry entry;
        entry.path = dirEntry.path();
        entry.name = dirEntry.path().filename().string();
        entry.isDirectory = dirEntry.is_directory(ec);
        if (!entry.isDirectory && AssetPath::IsMetadataSidecar(entry.path)) {
            continue;
        }
        entry.type = entry.isDirectory ? FileTypeInfo{EditorFileType::Folder, "Folder", true}
                                       : FileTypeRegistry::Classify(entry.path);
        if (MatchesSearchAndFilter(entry, true)) {
            if (results.size() >= static_cast<std::size_t>(kMaxSearchResults)) {
                if (truncated != nullptr) {
                    *truncated = true;
                }
                return results;
            }
            results.push_back(entry);
        }
    }
    return results;
}

bool ProjectPanel::MatchesSearchAndFilter(const ProjectEntry& entry, bool includeFolders) const {
    if (entry.isDirectory && !includeFolders) {
        return false;
    }
    if (!TypeMatches(m_TypeFilter, entry)) {
        return false;
    }
    if (!m_Browser.SearchActive()) {
        return true;
    }
    return m_Browser.MatchesSearch(entry.name) || m_Browser.MatchesSearch(entry.path.generic_string());
}

const AssetMetadata* ProjectPanel::SelectedMetadata(EditorContext& context) const {
    if (context.assetManager == nullptr || !m_SelectedAssetId.IsValid()) {
        return nullptr;
    }
    const AssetMetadata* meta = context.assetManager->Database().Find(m_SelectedAssetId);
    return meta != nullptr && IsInspectableAsset(*meta) ? meta : nullptr;
}

const AssetMetadata* ProjectPanel::MetadataForRawEntry(EditorContext& context, const ProjectEntry& entry) {
    if (entry.isDirectory || context.assetManager == nullptr) {
        return nullptr;
    }
    const AssetMetadata* meta = FindAssetMeta(context, entry.path);
    return meta != nullptr && IsInspectableAsset(*meta) ? meta : nullptr;
}

AssetType ProjectPanel::SelectedSectionAssetType() const {
    return SectionType(m_SelectedSection);
}

std::filesystem::path ProjectPanel::SelectedSectionRawFolder(EditorContext& context, AssetType fallbackType) const {
    const AssetType sectionType = SelectedSectionAssetType();
    const AssetType type = sectionType == AssetType::Unknown ? fallbackType : sectionType;
    const std::filesystem::path rawRoot = context.assetManager != nullptr ? context.assetManager->Info().rawRoot
                                                                          : Paths::Get().rawAssets;
    if (type == AssetType::Unknown) {
        return rawRoot;
    }
    return rawRoot / AssetPath::CookedSubdirectory(type);
}

void ProjectPanel::CreateMaterialAsset(EditorContext& context, const std::filesystem::path& directory) {
    std::error_code ec;
    const std::filesystem::path target =
        std::filesystem::is_directory(directory, ec) ? directory : directory.parent_path();

    std::filesystem::path path = target / "NewMaterial.material.yaml";
    int suffix = 1;
    while (std::filesystem::exists(path, ec)) {
        path = target / ("NewMaterial_" + std::to_string(suffix++) + ".material.yaml");
    }

    MaterialSource source;
    source.name = path.stem().stem().string();
    if (const Status status = MaterialSerializer::SaveFile(path, source); !status) {
        m_Status = status.error;
        return;
    }
    if (ImportAndCookRawAsset(context, path)) {
        m_Status = "Created " + path.filename().string();
    }
}

void ProjectPanel::CreateUIScreenAsset(EditorContext& context, const std::filesystem::path& directory) {
    std::error_code ec;
    const std::filesystem::path target =
        std::filesystem::is_directory(directory, ec) ? directory : directory.parent_path();
    if (const Status status = FileSystem::CreateDirectories(target); !status) {
        m_Status = status.error;
        return;
    }

    const std::filesystem::path path = UniqueDestination(target, "NewUIScreen.rml");
    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        m_Status = "Failed to create " + path.filename().string();
        return;
    }
    stream << "<rml>\n"
              "<head>\n"
              "  <link type=\"text/rcss\" href=\"data/ui/themes/hockey.rcss\"/>\n"
              "  <title>New UI Screen</title>\n"
              "</head>\n"
              "<body>\n"
              "  <div id=\"screen-root\">New UI Screen</div>\n"
              "</body>\n"
              "</rml>\n";
    if (!stream.good()) {
        m_Status = "Failed to write " + path.filename().string();
        return;
    }

    m_SelectedFolder = target;
    m_SelectedAssetId = {};
    context.ClearAssetSelection();
    m_Browser.Select(path);
    m_Status = "Created " + path.filename().string();
}

void ProjectPanel::CreateUIThemeAsset(EditorContext& context, const std::filesystem::path& directory) {
    std::error_code ec;
    const std::filesystem::path target =
        std::filesystem::is_directory(directory, ec) ? directory : directory.parent_path();
    if (const Status status = FileSystem::CreateDirectories(target); !status) {
        m_Status = status.error;
        return;
    }

    const std::filesystem::path path = UniqueDestination(target, "NewUITheme.rcss");
    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        m_Status = "Failed to create " + path.filename().string();
        return;
    }
    stream << "body {\n"
              "  font-family: Inter;\n"
              "  color: #f6f7f8;\n"
              "}\n";
    if (!stream.good()) {
        m_Status = "Failed to write " + path.filename().string();
        return;
    }

    m_SelectedFolder = target;
    m_SelectedAssetId = {};
    context.ClearAssetSelection();
    m_Browser.Select(path);
    m_Status = "Created " + path.filename().string();
}

void ProjectPanel::CreateClientFlowAsset(EditorContext& context, const std::filesystem::path& directory) {
    std::error_code ec;
    const std::filesystem::path target =
        std::filesystem::is_directory(directory, ec) ? directory : directory.parent_path();
    const std::filesystem::path path = UniqueDestination(target, "NewClientFlow.clientflow.yaml");
    if (const Status status = ClientFlowSerializer::Save(MakeDefaultClientFlow(), path); !status) {
        m_Status = status.error;
        return;
    }

    m_SelectedFolder = target;
    m_SelectedAssetId = {};
    context.ClearAssetSelection();
    m_Browser.Select(path);
    OpenClientFlowFile(context, path);
    m_Status = "Created " + path.filename().string();
}

void ProjectPanel::OpenSceneFile(EditorContext& context, const std::filesystem::path& path) {
    context.requestedOpenScene = path;
}

void ProjectPanel::OpenClientFlowFile(EditorContext& context, const std::filesystem::path& path) {
    context.clientFlowAssetPath = path;
    context.requestedPanelFocus = EditorPanelNames::kClientFlow;
}

void ProjectPanel::InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(EditorCommands::InstantiatePrefab(path), context);
}

std::vector<AssetID> ProjectPanel::AssetIdsUnderPath(EditorContext& context,
                                                     const std::filesystem::path& target) const {
    std::vector<AssetID> ids;
    if (context.assetManager == nullptr || target.empty()) {
        return ids;
    }

    for (const AssetMetadata* metadata : context.assetManager->Database().All()) {
        if (metadata == nullptr) {
            continue;
        }
        const std::filesystem::path raw = ResolveProjectPath(metadata->rawPath);
        if (SamePath(raw, target) || PathIsWithin(raw, target)) {
            ids.push_back(metadata->id);
        }
    }
    return ids;
}

Status ProjectPanel::DeleteProjectPath(EditorContext& context, const std::filesystem::path& target) {
    if (!CanRenameDeletePath(target)) {
        return Status::Fail("Project roots cannot be renamed or deleted from this menu");
    }

    const std::vector<AssetID> assetIds = AssetIdsUnderPath(context, target);
    for (const AssetID id : assetIds) {
        if (const Status status = context.assetManager->DeleteAssetFiles(id); !status) {
            return status;
        }
    }

    if (std::filesystem::exists(target)) {
        if (const Status status = m_Browser.DeleteEntry(target); !status) {
            return status;
        }
    }

    if (context.assetManager != nullptr) {
        if (const Status status = context.assetManager->DiscoverRawAssets(); !status) {
            return status;
        }
        if (const Status status = context.assetManager->CookAllDirty(); !status) {
            return status;
        }
        if (const Status status = context.assetManager->SaveDatabase(); !status) {
            return status;
        }
    }

    return Status::Ok();
}

void ProjectPanel::DrawModals(EditorContext& context) {
    if (m_OpenModalRequested) {
        switch (m_Modal) {
        case Modal::CreateFolder:
            ImGui::OpenPopup("New Folder");
            break;
        case Modal::Rename:
            ImGui::OpenPopup("Rename Item");
            break;
        case Modal::ConfirmDelete:
            ImGui::OpenPopup("Delete Item");
            break;
        case Modal::None:
            break;
        }
        m_OpenModalRequested = false;
    }

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Folder name:");
        ImGui::SetNextItemWidth(280.0f);
        const bool submit =
            ImGui::InputText("##foldername", m_NameBuffer, sizeof(m_NameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Create") || submit) {
            if (!m_Browser.IsWithinRoots(m_ModalTarget)) {
                m_Status = "Cannot create a folder outside project asset roots";
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
            if (const Status status = m_Browser.CreateFolder(m_ModalTarget, m_NameBuffer); !status) {
                m_Status = status.error;
            } else {
                m_SelectedFolder = m_ModalTarget / m_NameBuffer;
                m_Status.clear();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Rename Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("New name:");
        ImGui::SetNextItemWidth(280.0f);
        const bool submit = ImGui::InputText("##rename", m_NameBuffer, sizeof(m_NameBuffer),
                                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        if (ImGui::Button("Rename") || submit) {
            if (!CanRenameDeletePath(m_ModalTarget)) {
                m_Status = "Project roots cannot be renamed or deleted from this menu";
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
            const std::filesystem::path oldPath = m_ModalTarget;
            const std::filesystem::path newPath = oldPath.parent_path() / m_NameBuffer;
            if (const Status status = m_Browser.RenameEntry(oldPath, m_NameBuffer); !status) {
                m_Status = status.error;
            } else {
                if (SamePath(m_SelectedFolder, oldPath)) {
                    m_SelectedFolder = newPath;
                }
                if (context.assetManager != nullptr) {
                    context.assetManager->DiscoverRawAssets();
                    context.assetManager->CookAllDirty();
                    context.assetManager->SaveDatabase();
                }
                m_Status.clear();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Delete Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete \"%s\"?", m_ModalTarget.filename().string().c_str());
        ImGui::TextDisabled("This permanently removes the source file/folder from disk.");
        ImGui::Separator();
        if (ImGui::Button("Delete")) {
            if (!CanRenameDeletePath(m_ModalTarget)) {
                m_Status = "Project roots cannot be renamed or deleted from this menu";
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
            if (const Status status = DeleteProjectPath(context, m_ModalTarget); !status) {
                m_Status = status.error;
            } else {
                if (SamePath(m_SelectedFolder, m_ModalTarget) || PathIsWithin(m_SelectedFolder, m_ModalTarget)) {
                    m_SelectedFolder = m_Browser.ParentFolderWithinRoots(m_ModalTarget);
                }
                m_SelectedAssetId = {};
                InvalidateAssetEvents(context);
                m_Status.clear();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace Hockey
