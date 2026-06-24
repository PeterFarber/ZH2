#include "Hockey/Editor/Panels/ProjectPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
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
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

constexpr int kMaxSearchResults = 400;
constexpr float kListModeThreshold = 32.0f;

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

std::filesystem::path ProjectRelativePath(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    std::error_code ec;
    const std::filesystem::path relative = std::filesystem::relative(ResolveProjectPath(path), Paths::Get().root, ec);
    return ec ? path.lexically_normal() : relative.lexically_normal();
}

bool IsVisibleCookedAsset(const AssetMetadata& meta) {
    return meta.id.IsValid() && meta.cooked && !meta.missing && !meta.rawPath.empty() && !meta.cookedPath.empty();
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

std::string SectionDisplayLabel(const SectionInfo& info, const AssetDatabase* database, const ProjectBrowser& browser) {
    std::string label = info.label;
    if (database != nullptr) {
        label += " (" + std::to_string(browser.SectionEntries(database, info.type).size()) + ")";
    }
    return label;
}

bool TypeMatches(EditorFileType filter, const CookedProjectEntry& entry) {
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
        m_SelectedAssetId = {};
        m_SelectedFolder = SelectedSectionRawFolder(context, AssetType::Unknown);
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
    ImGui::InputTextWithHint("##project-search", "Search cooked assets...", m_Browser.SearchBuffer(),
                             m_Browser.SearchBufferSize());
    EditorTooltip::ForLastItem("Search cooked virtual folders and cooked assets by name or path");
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
    EditorTooltip::ForLastItem("Filter visible cooked entries by asset type");

    if (!m_Status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "%s", m_Status.c_str());
    }
}

float ProjectPanel::SectionListWidth(EditorContext& context) const {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    float maxLabelWidth = 0.0f;
    for (const SectionInfo& info : kSections) {
        const std::string label = SectionDisplayLabel(info, database, m_Browser);
        maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(label.c_str()).x);
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    return std::ceil(maxLabelWidth + (style.WindowPadding.x * 2.0f) + (style.FramePadding.x * 2.0f) +
                     (style.ChildBorderSize * 2.0f));
}

void ProjectPanel::DrawSectionList(EditorContext& context) {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    for (const SectionInfo& info : kSections) {
        const bool selected = m_SelectedSection == info.section;
        const std::string label = SectionDisplayLabel(info, database, m_Browser);

        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
            m_SelectedSection = info.section;
            m_SelectedAssetId = {};
            context.ClearAssetSelection();
            m_SelectedFolder = SelectedSectionRawFolder(context, info.type);
        }
        EditorTooltip::ForLastItem("Show cooked assets in this Project section");
    }
}

void ProjectPanel::DrawFolderContents(EditorContext& context) {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    if (database == nullptr) {
        ImGui::TextDisabled("No asset database is available.");
        return;
    }

    std::vector<CookedProjectEntry> entries;
    for (const CookedProjectEntry& entry : m_Browser.SectionEntries(database, SelectedSectionAssetType())) {
        if (MatchesSearchAndFilter(entry)) {
            entries.push_back(entry);
        }
    }
    if (entries.empty()) {
        ImGui::TextDisabled("No cooked assets in %s match the current filter.", SectionLabel(m_SelectedSection));
        return;
    }

    if (m_IconSize <= kListModeThreshold) {
        for (const CookedProjectEntry& entry : entries) {
            DrawCookedEntry(context, entry);
        }
        return;
    }

    const float cellWidth = std::max(96.0f, m_IconSize + 34.0f);
    const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));
    if (ImGui::BeginTable("##project-tile-grid", columns, ImGuiTableFlags_SizingFixedFit)) {
        for (const CookedProjectEntry& entry : entries) {
            ImGui::TableNextColumn();
            DrawCookedEntry(context, entry);
        }
        ImGui::EndTable();
    }
}

void ProjectPanel::DrawSearchResults(EditorContext& context) {
    bool truncated = false;
    const std::vector<CookedProjectEntry> entries = SearchEntries(context, &truncated);
    if (entries.empty()) {
        ImGui::TextDisabled("No cooked search results.");
        return;
    }

    for (const CookedProjectEntry& entry : entries) {
        DrawCookedEntry(context, entry);
    }
    if (truncated) {
        ImGui::TextDisabled("(results truncated)");
    }
}

void ProjectPanel::DrawCookedEntry(EditorContext& context, const CookedProjectEntry& entry) {
    ImGui::PushID(entry.isDirectory ? entry.virtualFolderPath.string().c_str() : entry.rawPath.string().c_str());
    const bool selected = entry.isDirectory ? SamePath(entry.virtualFolderPath, m_SelectedFolder)
                                            : (entry.assetId == m_SelectedAssetId);
    const ImVec4 color = ColorForType(entry.type.type, entry.type.supported);
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, color);

    if (m_IconSize <= kListModeThreshold) {
        const std::string label = EditorIconLabel(EditorIconForAssetType(entry.type.type), entry.displayName);
        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            if (entry.isDirectory) {
                m_SelectedFolder = entry.virtualFolderPath;
                m_SelectedAssetId = {};
                context.ClearAssetSelection();
            } else {
                m_SelectedAssetId = entry.assetId;
                m_Browser.Select(entry.rawPath);
                context.SelectAsset(entry.assetId);
                context.requestedPanelFocus = EditorPanelNames::kInspector;
            }
        }
        BeginCookedDragSource(context, entry);
        HandleCookedActivation(context, entry);
        DrawCookedContextMenu(context, entry);
    } else {
        const ImVec2 size(m_IconSize, m_IconSize);
        const EditorIcon icon = EditorIconForAssetType(entry.type.type);
        const char* glyph = EditorIconGlyph(icon);
        const char* tileLabel = (glyph != nullptr && glyph[0] != '\0') ? glyph : "Asset";
        if (ImGui::Button(tileLabel, size)) {
            if (entry.isDirectory) {
                m_SelectedFolder = entry.virtualFolderPath;
                m_SelectedAssetId = {};
                context.ClearAssetSelection();
            } else {
                m_SelectedAssetId = entry.assetId;
                m_Browser.Select(entry.rawPath);
                context.SelectAsset(entry.assetId);
                context.requestedPanelFocus = EditorPanelNames::kInspector;
            }
        }
        if (selected) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                ImGui::GetColorU32(ImGuiCol_Text));
        }
        BeginCookedDragSource(context, entry);
        HandleCookedActivation(context, entry);
        DrawCookedContextMenu(context, entry);
        ImGui::TextWrapped("%s", entry.displayName.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::PopID();
}

void ProjectPanel::DrawCookedContextMenu(EditorContext& context, const CookedProjectEntry& entry) {
    if (!ImGui::BeginPopupContextItem("##project-entry-context")) {
        return;
    }

    if (entry.isDirectory) {
        m_SelectedFolder = entry.virtualFolderPath;
        m_SelectedAssetId = {};
        if (ImGui::MenuItem("New Folder...")) {
            RequestModal(Modal::CreateFolder, entry.virtualFolderPath);
        }
        if (context.assetManager != nullptr && ImGui::MenuItem("New Material")) {
            CreateMaterialAsset(context, entry.virtualFolderPath);
        }
        if (ImGui::MenuItem("Rename...")) {
            RequestModal(Modal::Rename, entry.virtualFolderPath);
        }
        if (ImGui::MenuItem("Delete...")) {
            RequestModal(Modal::ConfirmDelete, entry.virtualFolderPath);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reveal Source")) {
            ProjectBrowser::Reveal(entry.virtualFolderPath);
        }
        ImGui::EndPopup();
        return;
    }

    m_SelectedAssetId = entry.assetId;
    context.SelectAsset(entry.assetId);
    context.requestedPanelFocus = EditorPanelNames::kInspector;
    if (entry.type.type == EditorFileType::Scene && ImGui::MenuItem("Open Scene")) {
        OpenSceneFile(context, entry.rawPath);
    }
    if (entry.type.type == EditorFileType::Prefab && ImGui::MenuItem("Instantiate Prefab")) {
        InstantiatePrefabFile(context, entry.rawPath);
    }
    if (entry.type.type == EditorFileType::Scene || entry.type.type == EditorFileType::Prefab) {
        ImGui::Separator();
    }
    if (context.assetManager != nullptr && ImGui::MenuItem("Reimport & Recook")) {
        ReimportAndRecook(context, entry);
    }
    EditorTooltip::ForLastItem("Reimport the raw source asset, recook dirty dependents, and save the database");
    if (context.assetManager != nullptr && ImGui::MenuItem("Recook")) {
        Recook(context, entry);
    }
    EditorTooltip::ForLastItem("Mark this cooked asset dirty and cook it immediately");
    if (context.assetManager != nullptr && ImGui::MenuItem("Validate References")) {
        std::vector<AssetValidationIssue> issues;
        const Status status = context.assetManager->ValidateReferences(&issues);
        m_Status = status ? "Validation passed (" + std::to_string(issues.size()) + " notes)"
                          : "Validation found issues: " + std::to_string(issues.size());
    }
    EditorTooltip::ForLastItem("Check asset database references for missing or stale IDs");
    if (ImGui::MenuItem("Copy Asset ID")) {
        ImGui::SetClipboardText(entry.assetId.ToString().c_str());
    }
    EditorTooltip::ForLastItem("Copy this cooked asset's stable ID to the clipboard");
    if (ImGui::MenuItem("Reveal Source")) {
        ProjectBrowser::Reveal(entry.rawPath);
    }
    EditorTooltip::ForLastItem("Reveal the raw source file in the system file manager");
    if (ImGui::MenuItem("Reveal Cooked")) {
        ProjectBrowser::Reveal(entry.cookedPath);
    }
    EditorTooltip::ForLastItem("Reveal the cooked output file in the system file manager");
    ImGui::Separator();
    if (ImGui::MenuItem("Rename...")) {
        RequestModal(Modal::Rename, entry.rawPath);
    }
    if (ImGui::MenuItem("Delete...")) {
        RequestModal(Modal::ConfirmDelete, entry.rawPath);
    }
    ImGui::EndPopup();
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

void ProjectPanel::BeginCookedDragSource(EditorContext& context, const CookedProjectEntry& entry) {
    if (entry.isDirectory) {
        return;
    }
    if (entry.type.type == EditorFileType::Prefab) {
        if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            return;
        }
        const std::string pathString = entry.rawPath.string();
        ImGui::SetDragDropPayload(kPrefabDragDropType, pathString.c_str(), pathString.size() + 1);
        ImGui::TextUnformatted(entry.displayName.c_str());
        ImGui::EndDragDropSource();
        return;
    }

    if (context.assetManager == nullptr || !entry.assetId.IsValid()) {
        return;
    }
    if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        return;
    }
    const AssetMetadata* meta = context.assetManager->Database().Find(entry.assetId);
    const AssetType type = meta != nullptr ? meta->type : AssetType::Unknown;
    const AssetDragPayload payload{entry.assetId.Value(), static_cast<int>(type)};
    ImGui::SetDragDropPayload(kAssetDragDropType, &payload, sizeof(payload));
    ImGui::Text("%s (%s)", entry.displayName.c_str(), AssetTypeToString(type).c_str());
    ImGui::EndDragDropSource();
}

void ProjectPanel::HandleCookedActivation(EditorContext& context, const CookedProjectEntry& entry) {
    if (!ImGui::IsItemHovered() || !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        return;
    }
    if (entry.isDirectory) {
        m_SelectedFolder = entry.virtualFolderPath;
        m_SelectedAssetId = {};
    } else if (entry.type.type == EditorFileType::Scene) {
        OpenSceneFile(context, entry.rawPath);
    } else if (entry.type.type == EditorFileType::Prefab) {
        InstantiatePrefabFile(context, entry.rawPath);
    }
}

void ProjectPanel::ReimportAndRecook(EditorContext& context, const CookedProjectEntry& entry) {
    if (context.assetManager == nullptr) {
        return;
    }
    if (const Status status = context.assetManager->ImportAsset(entry.rawPath); !status) {
        m_Status = status.error;
        return;
    }
    context.assetManager->Database().MarkDirtyWithDependents(entry.assetId);
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Reimported & recooked";
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
    if (context.assetManager->Database().Find(entry.assetId) != nullptr) {
        m_SelectedAssetId = entry.assetId;
    }
}

void ProjectPanel::Recook(EditorContext& context, const CookedProjectEntry& entry) {
    if (context.assetManager == nullptr) {
        return;
    }
    context.assetManager->Database().MarkDirty(entry.assetId);
    if (const Status status = context.assetManager->CookAsset(entry.assetId); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Recooked " + entry.displayName;
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
        if (IsVisibleCookedAsset(*meta)) {
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

std::vector<CookedProjectEntry> ProjectPanel::SearchEntries(EditorContext& context, bool* truncated) const {
    std::vector<CookedProjectEntry> results;
    if (truncated != nullptr) {
        *truncated = false;
    }
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    if (database == nullptr) {
        return results;
    }

    for (const CookedProjectEntry& entry : m_Browser.SectionEntries(database, SelectedSectionAssetType())) {
        if (MatchesSearchAndFilter(entry)) {
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

bool ProjectPanel::MatchesSearchAndFilter(const CookedProjectEntry& entry) const {
    if (!TypeMatches(m_TypeFilter, entry)) {
        return false;
    }
    if (!m_Browser.SearchActive()) {
        return true;
    }
    return m_Browser.MatchesSearch(entry.displayName) ||
           m_Browser.MatchesSearch(entry.rawPath.generic_string()) ||
           m_Browser.MatchesSearch(entry.cookedPath.generic_string());
}

const AssetMetadata* ProjectPanel::SelectedMetadata(EditorContext& context) const {
    if (context.assetManager == nullptr || !m_SelectedAssetId.IsValid()) {
        return nullptr;
    }
    const AssetMetadata* meta = context.assetManager->Database().Find(m_SelectedAssetId);
    return meta != nullptr && IsVisibleCookedAsset(*meta) ? meta : nullptr;
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

void ProjectPanel::OpenSceneFile(EditorContext& context, const std::filesystem::path& path) {
    context.requestedOpenScene = path;
}

void ProjectPanel::InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(EditorCommands::InstantiatePrefab(path), context);
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
            if (const Status status = m_Browser.DeleteEntry(m_ModalTarget); !status) {
                m_Status = status.error;
            } else {
                if (context.assetManager != nullptr) {
                    context.assetManager->DiscoverRawAssets();
                    context.assetManager->CookAllDirty();
                    context.assetManager->SaveDatabase();
                }
                if (SamePath(m_SelectedFolder, m_ModalTarget)) {
                    m_SelectedFolder = m_Browser.ParentFolderWithinRoots(m_ModalTarget);
                }
                m_SelectedAssetId = {};
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
