#include "Hockey/Editor/Panels/ProjectPanel.hpp"

#include <algorithm>
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
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/FileDialog.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Editor/Project/EditorAssetPreview.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

constexpr int kMaxSearchResults = 400;
constexpr float kListModeThreshold = 32.0f;

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

const char* TypeGlyph(EditorFileType type) {
    switch (type) {
    case EditorFileType::Folder:
        return "[Folder]";
    case EditorFileType::Scene:
        return "[Scene]";
    case EditorFileType::Prefab:
        return "[Prefab]";
    case EditorFileType::Material:
        return "[Material]";
    case EditorFileType::Image:
        return "[Image]";
    case EditorFileType::Model:
        return "[Model]";
    case EditorFileType::ShaderSource:
        return "[Shader]";
    default:
        return "[Asset]";
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
            const float treeWidth = std::max(180.0f, ImGui::GetContentRegionAvail().x * 0.32f);
            if (ImGui::BeginChild("##project-folder-tree", ImVec2(treeWidth, 0.0f), true)) {
                DrawFolderTree(context);
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("##project-right", ImVec2(0.0f, 0.0f), false)) {
                const float detailsHeight = 190.0f;
                const float contentsHeight = std::max(120.0f, ImGui::GetContentRegionAvail().y - detailsHeight - 6.0f);
                if (ImGui::BeginChild("##project-contents", ImVec2(0.0f, contentsHeight), true)) {
                    if (m_Browser.SearchActive()) {
                        DrawSearchResults(context);
                    } else {
                        DrawFolderContents(context);
                    }
                }
                ImGui::EndChild();

                if (ImGui::BeginChild("##project-details", ImVec2(0.0f, 0.0f), true)) {
                    DrawDetails(context);
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
    if (m_SelectedFolder.empty() || !m_Browser.IsWithinRoots(m_SelectedFolder)) {
        if (!m_Browser.Roots().empty()) {
            m_SelectedFolder = m_Browser.Roots().front().path;
        }
    }

    if (const AssetMetadata* meta = SelectedMetadata(context)) {
        m_Browser.Select(ResolveProjectPath(meta->rawPath));
    } else {
        m_SelectedAssetId = {};
    }
}

void ProjectPanel::DrawToolbar(EditorContext& context) {
    if (ImGui::Button("Create")) {
        ImGui::OpenPopup("##project-create");
    }
    EditorTooltip::ForLastItem("Create source assets under the selected Project folder");
    if (ImGui::BeginPopup("##project-create")) {
        if (ImGui::MenuItem("New Folder...")) {
            RequestModal(Modal::CreateFolder, m_SelectedFolder);
        }
        EditorTooltip::ForLastItem("Create a source folder in the selected virtual folder");
        ImGui::BeginDisabled(context.assetManager == nullptr);
        if (ImGui::MenuItem("New Material")) {
            CreateMaterialAsset(context, m_SelectedFolder);
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

    static const char* const kScopes[] = {"All", "Selected Root", "Selected Folder"};
    int scope = static_cast<int>(m_SearchScope);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::Combo("##project-search-scope", &scope, kScopes, 3)) {
        m_SearchScope = static_cast<ProjectSearchScope>(scope);
    }
    EditorTooltip::ForLastItem("Choose whether search scans all cooked roots, the selected root, or this folder");

    struct TypeOption {
        EditorFileType type;
        const char* label;
    };
    static constexpr TypeOption kTypes[] = {
        {EditorFileType::Unknown, "All"},
        {EditorFileType::Folder, "Folder"},
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

void ProjectPanel::DrawFolderTree(EditorContext& context) {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    for (const ProjectBrowser::Root& root : m_Browser.Roots()) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (SamePath(root.path, m_SelectedFolder)) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (&root == &m_Browser.Roots().front()) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(EditorFileType::Folder, true));
        const bool open = ImGui::TreeNodeEx(root.path.string().c_str(), flags, "%s", root.label.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedFolder = root.path;
            m_SelectedAssetId = {};
        }
        if (open) {
            if (database == nullptr) {
                ImGui::TextDisabled("(no asset database)");
            } else {
                for (const CookedProjectEntry& entry : m_Browser.Entries(root.path, database)) {
                    if (entry.isDirectory) {
                        DrawFolderNode(context, entry);
                    }
                }
            }
            ImGui::TreePop();
        }
    }
}

void ProjectPanel::DrawFolderNode(EditorContext& context, const CookedProjectEntry& folder) {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (SamePath(folder.virtualFolderPath, m_SelectedFolder)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(EditorFileType::Folder, true));
    const bool open = ImGui::TreeNodeEx(folder.virtualFolderPath.string().c_str(), flags, "%s", folder.displayName.c_str());
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        m_SelectedFolder = folder.virtualFolderPath;
        m_SelectedAssetId = {};
    }
    DrawCookedContextMenu(context, folder);
    if (open) {
        if (database != nullptr) {
            for (const CookedProjectEntry& child : m_Browser.Entries(folder.virtualFolderPath, database)) {
                if (child.isDirectory) {
                    DrawFolderNode(context, child);
                }
            }
        }
        ImGui::TreePop();
    }
}

void ProjectPanel::DrawFolderContents(EditorContext& context) {
    const AssetDatabase* database = context.assetManager != nullptr ? &context.assetManager->Database() : nullptr;
    if (database == nullptr) {
        ImGui::TextDisabled("No asset database is available.");
        return;
    }

    std::vector<CookedProjectEntry> entries;
    for (const CookedProjectEntry& entry : m_Browser.Entries(m_SelectedFolder, database)) {
        if (MatchesSearchAndFilter(entry)) {
            entries.push_back(entry);
        }
    }
    if (entries.empty()) {
        ImGui::TextDisabled("Empty cooked folder: no immediate cooked entries match the current filter.");
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
        const std::string label = std::string(TypeGlyph(entry.type.type)) + " " + entry.displayName;
        if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            if (entry.isDirectory) {
                m_SelectedFolder = entry.virtualFolderPath;
                m_SelectedAssetId = {};
            } else {
                m_SelectedAssetId = entry.assetId;
                m_Browser.Select(entry.rawPath);
            }
        }
    } else {
        const ImVec2 size(m_IconSize, m_IconSize);
        if (ImGui::Button(TypeGlyph(entry.type.type), size)) {
            if (entry.isDirectory) {
                m_SelectedFolder = entry.virtualFolderPath;
                m_SelectedAssetId = {};
            } else {
                m_SelectedAssetId = entry.assetId;
                m_Browser.Select(entry.rawPath);
            }
        }
        if (selected) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                ImGui::GetColorU32(ImGuiCol_Text));
        }
        ImGui::TextWrapped("%s", entry.displayName.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::EndGroup();

    BeginCookedDragSource(context, entry);
    HandleCookedActivation(context, entry);
    DrawCookedContextMenu(context, entry);
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

void ProjectPanel::DrawDetails(EditorContext& context) {
    if (const AssetMetadata* meta = SelectedMetadata(context)) {
        const std::filesystem::path raw = ResolveProjectPath(meta->rawPath);
        const std::filesystem::path cooked = ResolveProjectPath(meta->cookedPath);
        ImGui::TextUnformatted(meta->name.empty() ? raw.filename().string().c_str() : meta->name.c_str());
        ImGui::TextDisabled("Type: %s", AssetTypeToString(meta->type).c_str());
        ImGui::TextDisabled("ID: %s", meta->id.ToString().c_str());
        ImGui::TextDisabled("Raw: %s", ProjectRelativePath(raw).generic_string().c_str());
        ImGui::TextDisabled("Cooked: %s", ProjectRelativePath(cooked).generic_string().c_str());
        ImGui::TextDisabled("Status: %s", AssetStatusLabel(*meta));
        if (!meta->dependencies.empty()) {
            ImGui::TextDisabled("Dependencies: %zu", meta->dependencies.size());
        }
        if (meta->type == AssetType::Material) {
            DrawMaterialEditor(context, raw);
        } else {
            const EditorAssetPreview::Preview preview = EditorAssetPreview::Describe(raw);
            if (preview.hasSnippet) {
                ImGui::Separator();
                ImGui::PushTextWrapPos(0.0f);
                ImGui::TextDisabled("%s", preview.snippet.c_str());
                ImGui::PopTextWrapPos();
            }
        }
        return;
    }

    ImGui::TextUnformatted(m_SelectedFolder.filename().empty() ? m_SelectedFolder.string().c_str()
                                                               : m_SelectedFolder.filename().string().c_str());
    ImGui::TextDisabled("Folder");
    ImGui::TextDisabled("%s", ProjectRelativePath(m_SelectedFolder).generic_string().c_str());
    if (ImGui::Button("New Folder...")) {
        RequestModal(Modal::CreateFolder, m_SelectedFolder);
    }
    EditorTooltip::ForLastItem("Create a source folder under the selected virtual folder");
}

void ProjectPanel::DrawStatusStrip(EditorContext& context) {
    const AssetMetadata* meta = SelectedMetadata(context);
    if (meta != nullptr) {
        const std::filesystem::path raw = ResolveProjectPath(meta->rawPath);
        const std::filesystem::path cooked = ResolveProjectPath(meta->cookedPath);
        ImGui::Text("Selected: %s | Raw: %s | Cooked: %s", raw.filename().string().c_str(),
                    ProjectRelativePath(raw).generic_string().c_str(), ProjectRelativePath(cooked).generic_string().c_str());
    } else if (m_Browser.SearchActive()) {
        ImGui::Text("Search in %s: %s", ProjectRelativePath(m_SelectedFolder).generic_string().c_str(),
                    m_Browser.SearchBuffer());
    } else {
        ImGui::Text("Folder: %s", ProjectRelativePath(m_SelectedFolder).generic_string().c_str());
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

    std::filesystem::path targetDir = m_SelectedFolder;
    if (!m_Browser.IsWithinRoots(targetDir) || !SamePath(m_Browser.RootForPath(targetDir), context.assetManager->Info().rawRoot)) {
        targetDir = context.assetManager->Info().rawRoot / AssetPath::CookedSubdirectory(type);
    }
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
            m_SelectedFolder = ResolveProjectPath(meta->rawPath).parent_path();
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

    std::vector<std::filesystem::path> roots;
    switch (m_SearchScope) {
    case ProjectSearchScope::All:
        for (const ProjectBrowser::Root& root : m_Browser.Roots()) {
            roots.push_back(root.path);
        }
        break;
    case ProjectSearchScope::SelectedRoot:
        roots.push_back(m_Browser.RootForPath(m_SelectedFolder));
        break;
    case ProjectSearchScope::SelectedFolder:
        roots.push_back(m_SelectedFolder);
        break;
    }

    auto walk = [&](auto&& self, const std::filesystem::path& folder) -> void {
        if (folder.empty() || (truncated != nullptr && *truncated)) {
            return;
        }
        for (const CookedProjectEntry& entry : m_Browser.Entries(folder, database)) {
            if (MatchesSearchAndFilter(entry)) {
                if (results.size() >= static_cast<std::size_t>(kMaxSearchResults)) {
                    if (truncated != nullptr) {
                        *truncated = true;
                    }
                    return;
                }
                results.push_back(entry);
            }
            if (entry.isDirectory) {
                self(self, entry.virtualFolderPath);
            }
        }
    };

    for (const std::filesystem::path& root : roots) {
        walk(walk, root);
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

void ProjectPanel::DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path) {
    if (m_EditMaterialPath != path) {
        if (m_MaterialPreviewActive && context.renderer != nullptr && m_EditMaterialId != 0) {
            context.renderer->InvalidateAsset(m_EditMaterialId);
        }
        Result<MaterialSource> loaded = MaterialSerializer::LoadFile(path);
        m_EditMaterial = loaded ? loaded.value : MaterialSource{};
        m_EditMaterialPath = path;
        const AssetMetadata* meta = FindAssetMeta(context, path);
        m_EditMaterialId = meta != nullptr ? meta->id.Value() : 0;
        m_MaterialPreviewActive = false;
    }

    bool changed = false;

    ImGui::SeparatorText("Material");
    char nameBuf[256];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", m_EditMaterial.name.c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        m_EditMaterial.name = nameBuf;
    }
    EditorTooltip::ForLastItem("Material display name written to the source asset");

    ImGui::SeparatorText("Surface");
    changed |= ImGui::ColorEdit4("Albedo", &m_EditMaterial.baseColor.x);
    EditorTooltip::ForLastItem("Base color and alpha");
    changed |= ImGui::SliderFloat("Metallic", &m_EditMaterial.metallic, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Metallic surface response");
    changed |= ImGui::SliderFloat("Roughness", &m_EditMaterial.roughness, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Surface microsurface roughness");
    changed |= ImGui::SliderFloat("Normal Strength", &m_EditMaterial.normalStrength, 0.0f, 4.0f);
    EditorTooltip::ForLastItem("Strength applied to the normal map");
    changed |= ImGui::SliderFloat("Occlusion", &m_EditMaterial.occlusionStrength, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Ambient occlusion texture strength");
    changed |= ImGui::ColorEdit3("Emission", &m_EditMaterial.emissiveColor.x,
                                 ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
    EditorTooltip::ForLastItem("Emissive color contribution");
    changed |= ImGui::SliderFloat("Emission Strength", &m_EditMaterial.emissiveStrength, 0.0f, 16.0f);
    EditorTooltip::ForLastItem("Emissive intensity multiplier");

    {
        static const char* const kAlphaModes[] = {"Opaque", "Mask", "Blend"};
        int alphaModeIndex = 0;
        for (int i = 0; i < 3; ++i) {
            if (m_EditMaterial.alphaMode == kAlphaModes[i]) {
                alphaModeIndex = i;
                break;
            }
        }
        if (ImGui::Combo("Rendering Mode", &alphaModeIndex, kAlphaModes, 3)) {
            m_EditMaterial.alphaMode = kAlphaModes[alphaModeIndex];
            changed = true;
        }
        EditorTooltip::ForLastItem("Opaque ignores alpha, Mask clips by cutoff, Blend uses alpha blending");
        ImGui::BeginDisabled(m_EditMaterial.alphaMode != "Mask");
        changed |= ImGui::SliderFloat("Alpha Cutoff", &m_EditMaterial.alphaCutoff, 0.0f, 1.0f);
        EditorTooltip::ForLastItem("Alpha threshold used by Mask rendering mode");
        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Maps");
    ImGui::TextDisabled("Drag a texture asset from the tree onto a slot.");

    auto textureSlot = [&](const char* label, std::string& slot) -> bool {
        bool slotChanged = false;
        ImGui::PushID(label);

        std::uint64_t thumbId = 0;
        if (!slot.empty() && context.assetManager != nullptr && context.imguiBridge != nullptr) {
            if (const AssetMetadata* meta = context.assetManager->Database().FindByRawPath(std::filesystem::path(slot))) {
                thumbId = context.imguiBridge->TextureIdForAsset(meta->id.Value());
            }
        }

        const ImVec2 thumbSize(48.0f, 48.0f);
        if (thumbId != 0) {
            ImGui::Image(static_cast<ImTextureID>(thumbId), thumbSize);
        } else {
            ImGui::Button("##empty", thumbSize);
        }
        EditorTooltip::ForLastItem("Drop a texture asset here to assign this map");
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropType);
                payload != nullptr && payload->DataSize == static_cast<int>(sizeof(AssetDragPayload))) {
                const auto* drag = static_cast<const AssetDragPayload*>(payload->Data);
                if (drag->type == static_cast<int>(AssetType::Texture) && context.assetManager != nullptr) {
                    if (const AssetMetadata* meta = context.assetManager->Database().Find(AssetID{drag->id})) {
                        slot = meta->rawPath.generic_string();
                        slotChanged = true;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        if (slot.empty()) {
            ImGui::TextDisabled("(none)");
        } else {
            ImGui::TextDisabled("%s", std::filesystem::path(slot).filename().string().c_str());
            EditorTooltip::ForLastItem(slot.c_str());
            if (ImGui::SmallButton("Clear")) {
                slot.clear();
                slotChanged = true;
            }
            EditorTooltip::ForLastItem("Clear this texture slot");
        }
        ImGui::EndGroup();

        ImGui::PopID();
        return slotChanged;
    };

    changed |= textureSlot("Albedo", m_EditMaterial.baseColorTexture);
    changed |= textureSlot("Normal", m_EditMaterial.normalTexture);
    changed |= textureSlot("Metallic / Roughness", m_EditMaterial.metallicRoughnessTexture);
    changed |= textureSlot("Occlusion", m_EditMaterial.occlusionTexture);
    changed |= textureSlot("Emission", m_EditMaterial.emissiveTexture);

    changed |= ImGui::DragFloat2("Tiling", &m_EditMaterial.tiling.x, 0.01f, 0.0f, 256.0f);
    EditorTooltip::ForLastItem("UV tiling applied to every texture map");
    changed |= ImGui::DragFloat2("Offset", &m_EditMaterial.offset.x, 0.005f, -256.0f, 256.0f);
    EditorTooltip::ForLastItem("UV offset applied to every texture map");

    if (changed) {
        ApplyMaterialPreview(context);
    }

    ImGui::Separator();
    if (ImGui::Button("Save")) {
        if (const Status status = MaterialSerializer::SaveFile(path, m_EditMaterial); !status) {
            m_Status = status.error;
        } else if (context.assetManager != nullptr) {
            context.assetManager->ImportAsset(path);
            if (const AssetMetadata* meta = FindAssetMeta(context, path)) {
                const AssetID id = meta->id;
                context.assetManager->Database().MarkDirtyWithDependents(id);
                context.assetManager->CookAllDirty();
                context.assetManager->SaveDatabase();
                if (context.renderer != nullptr) {
                    context.renderer->InvalidateAsset(id.Value());
                }
                m_SelectedAssetId = id;
            }
            m_MaterialPreviewActive = false;
            m_Status = "Saved material";
        } else {
            m_Status = "Saved material (no asset manager to cook)";
        }
    }
    EditorTooltip::ForLastItem("Save material edits, reimport, and cook dependent assets");
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_MaterialPreviewActive);
    if (ImGui::Button("Revert")) {
        Result<MaterialSource> loaded = MaterialSerializer::LoadFile(path);
        m_EditMaterial = loaded ? loaded.value : MaterialSource{};
        if (context.renderer != nullptr && m_EditMaterialId != 0) {
            context.renderer->InvalidateAsset(m_EditMaterialId);
        }
        m_MaterialPreviewActive = false;
        m_Status = "Reverted";
    }
    EditorTooltip::ForLastItem("Discard unsaved live preview edits and reload from disk");
    ImGui::EndDisabled();
}

void ProjectPanel::ApplyMaterialPreview(EditorContext& context) {
    if (context.renderer == nullptr || context.assetManager == nullptr || m_EditMaterialId == 0) {
        return;
    }
    auto textureId = [&](const std::string& rawPath) -> std::uint64_t {
        if (rawPath.empty()) {
            return 0;
        }
        const AssetMetadata* meta = context.assetManager->Database().FindByRawPath(std::filesystem::path(rawPath));
        return meta != nullptr ? meta->id.Value() : 0;
    };

    MaterialPreviewDesc desc;
    desc.baseColor = m_EditMaterial.baseColor;
    desc.metallic = m_EditMaterial.metallic;
    desc.roughness = m_EditMaterial.roughness;
    desc.normalStrength = m_EditMaterial.normalStrength;
    desc.occlusionStrength = m_EditMaterial.occlusionStrength;
    desc.emissiveColor = m_EditMaterial.emissiveColor;
    desc.emissiveStrength = m_EditMaterial.emissiveStrength;
    desc.alphaMode = m_EditMaterial.alphaMode == "Mask"    ? AlphaMode::Mask
                     : m_EditMaterial.alphaMode == "Blend" ? AlphaMode::Blend
                                                           : AlphaMode::Opaque;
    desc.alphaCutoff = m_EditMaterial.alphaCutoff;
    desc.tiling = m_EditMaterial.tiling;
    desc.offset = m_EditMaterial.offset;
    desc.baseColorTexture = textureId(m_EditMaterial.baseColorTexture);
    desc.normalTexture = textureId(m_EditMaterial.normalTexture);
    desc.metallicRoughnessTexture = textureId(m_EditMaterial.metallicRoughnessTexture);
    desc.occlusionTexture = textureId(m_EditMaterial.occlusionTexture);
    desc.emissiveTexture = textureId(m_EditMaterial.emissiveTexture);

    context.renderer->PreviewMaterial(m_EditMaterialId, desc);
    m_MaterialPreviewActive = true;
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
