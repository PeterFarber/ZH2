#include "Hockey/Editor/Panels/ProjectPanel.hpp"

#include <cstdio>
#include <functional>
#include <system_error>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Editor/Project/EditorAssetPreview.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

constexpr int kMaxSearchResults = 400;

// Looks up the asset-database record for an absolute project file, or nullptr if
// no AssetManager is wired or the file is not a tracked asset.
const AssetMetadata* FindAssetMeta(EditorContext& context, const std::filesystem::path& absolute) {
    if (context.assetManager == nullptr) {
        return nullptr;
    }
    std::error_code ec;
    const std::filesystem::path relative = std::filesystem::relative(absolute, Paths::Get().root, ec);
    if (ec || relative.empty()) {
        return nullptr;
    }
    return context.assetManager->Database().FindByRawPath(relative);
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

// Folder icon glyphs are avoided (no icon font yet); we prefix with a small
// bracketed tag so directories and typed files are distinguishable in text.
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
    default:
        return supported ? ImVec4(0.85f, 0.85f, 0.85f, 1.0f) : ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
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
        m_Browser.ClearSelectionIfMissing();
        DrawToolbar();
        ImGui::Separator();

        const float detailsHeight = 132.0f;
        const float treeHeight = ImGui::GetContentRegionAvail().y - detailsHeight;
        if (ImGui::BeginChild("##project-tree", ImVec2(0.0f, treeHeight > 0.0f ? treeHeight : 0.0f), true)) {
            if (m_Browser.SearchActive()) {
                DrawSearchResults(context);
            } else {
                DrawTree(context);
            }
        }
        ImGui::EndChild();

        DrawDetails(context);
        DrawModals(context);
    }
    EndWindow();
}

void ProjectPanel::DrawToolbar() {
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##search", "Search...", m_Browser.SearchBuffer(), m_Browser.SearchBufferSize());
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        m_Browser.SearchBuffer()[0] = '\0';
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        m_Browser.ClearSelectionIfMissing();
        m_Status.clear();
    }
    if (!m_Status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "%s", m_Status.c_str());
    }
}

void ProjectPanel::DrawTree(EditorContext& context) {
    for (const ProjectBrowser::Root& root : m_Browser.Roots()) {
        std::error_code ec;
        const bool exists = std::filesystem::is_directory(root.path, ec);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (&root == &m_Browser.Roots().front()) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(EditorFileType::Folder, true));
        const bool open = ImGui::TreeNodeEx(root.label.c_str(), flags);
        ImGui::PopStyleColor();
        if (open) {
            if (exists) {
                DrawDirectoryChildren(context, root.path);
            } else {
                ImGui::TextDisabled("(missing)");
            }
            ImGui::TreePop();
        }
    }
}

void ProjectPanel::DrawDirectoryChildren(EditorContext& context, const std::filesystem::path& dir) {
    for (const ProjectEntry& entry : m_Browser.Entries(dir)) {
        if (entry.isDirectory) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (m_Browser.Selected() == entry.path) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(EditorFileType::Folder, true));
            const bool open = ImGui::TreeNodeEx(entry.path.string().c_str(), flags, "%s", entry.name.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_Browser.Select(entry.path);
            }
            DrawContextMenu(context, entry);
            if (open) {
                DrawDirectoryChildren(context, entry.path);
                ImGui::TreePop();
            }
        } else {
            DrawFileLeaf(context, entry);
        }
    }
}

void ProjectPanel::DrawFileLeaf(EditorContext& context, const ProjectEntry& entry) {
    const bool selected = m_Browser.Selected() == entry.path;
    ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(entry.type.type, entry.type.supported));
    if (ImGui::Selectable(entry.name.c_str(), selected,
                          ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
        m_Browser.Select(entry.path);
    }
    ImGui::PopStyleColor();
    BeginFileDragSource(context, entry);
    HandleFileActivation(context, entry);
    if (ImGui::IsItemHovered() && !entry.type.supported) {
        ImGui::SetTooltip("%s (preview only, not imported)", entry.type.label);
    }
    DrawContextMenu(context, entry);
}

void ProjectPanel::DrawSearchResults(EditorContext& context) {
    int shown = 0;
    bool truncated = false;
    std::function<void(const std::filesystem::path&)> walk = [&](const std::filesystem::path& dir) {
        if (truncated) {
            return;
        }
        for (const ProjectEntry& entry : m_Browser.Entries(dir)) {
            if (truncated) {
                return;
            }
            if (entry.isDirectory) {
                walk(entry.path);
            } else if (m_Browser.MatchesSearch(entry.name)) {
                if (shown >= kMaxSearchResults) {
                    truncated = true;
                    return;
                }
                ++shown;
                const bool selected = m_Browser.Selected() == entry.path;
                ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(entry.type.type, entry.type.supported));
                if (ImGui::Selectable(entry.path.string().c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    m_Browser.Select(entry.path);
                }
                ImGui::PopStyleColor();
                BeginFileDragSource(context, entry);
                HandleFileActivation(context, entry);
                DrawContextMenu(context, entry);
            }
        }
    };
    for (const ProjectBrowser::Root& root : m_Browser.Roots()) {
        walk(root.path);
    }
    if (shown == 0) {
        ImGui::TextDisabled("No matching files.");
    } else if (truncated) {
        ImGui::TextDisabled("(results truncated)");
    }
}

void ProjectPanel::DrawContextMenu(EditorContext& context, const ProjectEntry& entry) {
    if (!ImGui::BeginPopupContextItem(entry.path.string().c_str())) {
        return;
    }
    m_Browser.Select(entry.path);
    if (entry.type.type == EditorFileType::Scene && ImGui::MenuItem("Open Scene")) {
        OpenSceneFile(context, entry.path);
    }
    if (entry.type.type == EditorFileType::Prefab && ImGui::MenuItem("Instantiate Prefab")) {
        InstantiatePrefabFile(context, entry.path);
    }
    if ((entry.type.type == EditorFileType::Scene || entry.type.type == EditorFileType::Prefab)) {
        ImGui::Separator();
    }
    if (entry.isDirectory && ImGui::MenuItem("New Folder...")) {
        RequestModal(Modal::CreateFolder, entry.path);
    }
    if (entry.isDirectory && context.assetManager != nullptr && ImGui::MenuItem("New Material")) {
        CreateMaterialAsset(context, entry.path);
    }
    if (ImGui::MenuItem("Rename...")) {
        RequestModal(Modal::Rename, entry.path);
    }
    if (ImGui::MenuItem("Delete...")) {
        RequestModal(Modal::ConfirmDelete, entry.path);
    }
    if (!entry.isDirectory) {
        DrawAssetActions(context, entry.path);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Reveal in File Manager")) {
        ProjectBrowser::Reveal(entry.path);
    }
    ImGui::EndPopup();
}

void ProjectPanel::DrawAssetActions(EditorContext& context, const std::filesystem::path& path) {
    AssetManager* assets = context.assetManager;
    if (assets == nullptr) {
        return;
    }
    const AssetType type = AssetManager::ClassifyExtension(path);
    if (type == AssetType::Unknown) {
        return; // unsupported file: no asset actions
    }

    ImGui::Separator();
    const AssetMetadata* meta = FindAssetMeta(context, path);
    const bool imported = meta != nullptr;

    if (ImGui::MenuItem(imported ? "Reimport" : "Import")) {
        if (const Status status = assets->ImportAsset(path); !status) {
            m_Status = status.error;
        } else {
            assets->SaveDatabase();
            m_Status.clear();
        }
    }
    if (imported && ImGui::MenuItem("Cook")) {
        if (const Status status = assets->CookAsset(meta->id); !status) {
            m_Status = status.error;
        } else {
            m_Status.clear();
        }
    }
    if (imported && ImGui::MenuItem("Recook")) {
        assets->Database().MarkDirty(meta->id);
        if (const Status status = assets->CookAsset(meta->id); !status) {
            m_Status = status.error;
        } else {
            m_Status.clear();
        }
    }
    if (ImGui::MenuItem("Recook All Dirty")) {
        if (const Status status = assets->CookAllDirty(); !status) {
            m_Status = status.error;
        } else {
            m_Status.clear();
        }
        assets->SaveDatabase();
    }
    if (imported && ImGui::MenuItem("Copy Asset ID")) {
        ImGui::SetClipboardText(meta->id.ToString().c_str());
    }
    if (ImGui::MenuItem("Validate References")) {
        std::vector<AssetValidationIssue> issues;
        const Status status = assets->ValidateReferences(&issues);
        m_Status = status ? "Validation passed (" + std::to_string(issues.size()) + " notes)"
                          : "Validation found issues: " + std::to_string(issues.size());
    }
}

void ProjectPanel::DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path) {
    // Lazily (re)load the material whenever the selected file changes.
    if (m_EditMaterialPath != path) {
        Result<MaterialSource> loaded = MaterialSerializer::LoadFile(path);
        m_EditMaterial = loaded ? loaded.value : MaterialSource{};
        m_EditMaterialPath = path;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Material");

    char nameBuf[256];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", m_EditMaterial.name.c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        m_EditMaterial.name = nameBuf;
    }

    ImGui::ColorEdit4("Base Color", &m_EditMaterial.baseColor.x);
    ImGui::DragFloat("Metallic", &m_EditMaterial.metallic, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Roughness", &m_EditMaterial.roughness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Normal Strength", &m_EditMaterial.normalStrength, 0.01f, 0.0f, 4.0f);
    ImGui::DragFloat("Occlusion", &m_EditMaterial.occlusionStrength, 0.01f, 0.0f, 1.0f);
    ImGui::ColorEdit3("Emissive", &m_EditMaterial.emissiveColor.x);
    ImGui::DragFloat("Emissive Strength", &m_EditMaterial.emissiveStrength, 0.01f, 0.0f, 16.0f);

    if (ImGui::Button("Save")) {
        if (const Status status = MaterialSerializer::SaveFile(path, m_EditMaterial); !status) {
            m_Status = status.error;
        } else if (context.assetManager != nullptr) {
            // Reimport + recook so the renderer picks up the edit on next resolve.
            context.assetManager->ImportAsset(path);
            if (const AssetMetadata* meta = FindAssetMeta(context, path)) {
                const AssetID id = meta->id;
                context.assetManager->Database().MarkDirtyWithDependents(id);
                context.assetManager->CookAllDirty();
                context.assetManager->SaveDatabase();
                if (context.renderer != nullptr) {
                    context.renderer->InvalidateAsset(id.Value());
                }
            }
            m_Status = "Saved material";
        } else {
            m_Status = "Saved material (no asset manager to cook)";
        }
    }
}

void ProjectPanel::CreateMaterialAsset(EditorContext& context, const std::filesystem::path& directory) {
    std::error_code ec;
    const std::filesystem::path target =
        std::filesystem::is_directory(directory, ec) ? directory : directory.parent_path();

    // Pick a non-clobbering file name (NewMaterial.material.yaml, _1, _2, ...).
    std::filesystem::path path = target / "NewMaterial.material.yaml";
    int suffix = 1;
    while (std::filesystem::exists(path, ec)) {
        path = target / ("NewMaterial_" + std::to_string(suffix++) + ".material.yaml");
    }

    MaterialSource source;
    source.name = path.stem().stem().string(); // strip .material.yaml
    if (const Status status = MaterialSerializer::SaveFile(path, source); !status) {
        m_Status = status.error;
        return;
    }
    if (context.assetManager != nullptr) {
        context.assetManager->ImportAsset(path);
        context.assetManager->CookAllDirty();
        context.assetManager->SaveDatabase();
    }
    m_Browser.Select(path);
    m_Status = "Created " + path.filename().string();
}

void ProjectPanel::BeginFileDragSource(EditorContext& context, const ProjectEntry& entry) {
    // Prefabs drag as a path payload (the hierarchy/viewport instantiate them).
    if (entry.type.type == EditorFileType::Prefab) {
        if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            return;
        }
        const std::string pathString = entry.path.string();
        // Include the trailing null so the target can read it as a C string.
        ImGui::SetDragDropPayload(kPrefabDragDropType, pathString.c_str(), pathString.size() + 1);
        ImGui::TextUnformatted(entry.name.c_str());
        ImGui::EndDragDropSource();
        return;
    }

    // Any other tracked asset drags as an asset-id payload (inspector fields).
    const AssetMetadata* meta = FindAssetMeta(context, entry.path);
    if (meta == nullptr) {
        return;
    }
    if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        return;
    }
    const AssetDragPayload payload{meta->id.Value(), static_cast<int>(meta->type)};
    ImGui::SetDragDropPayload(kAssetDragDropType, &payload, sizeof(payload));
    ImGui::Text("%s (%s)", entry.name.c_str(), AssetTypeToString(meta->type).c_str());
    ImGui::EndDragDropSource();
}

void ProjectPanel::HandleFileActivation(EditorContext& context, const ProjectEntry& entry) {
    if (!ImGui::IsItemHovered() || !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        return;
    }
    if (entry.type.type == EditorFileType::Scene) {
        OpenSceneFile(context, entry.path);
    } else if (entry.type.type == EditorFileType::Prefab) {
        InstantiatePrefabFile(context, entry.path);
    }
}

void ProjectPanel::OpenSceneFile(EditorContext& context, const std::filesystem::path& path) {
    // Defer to the host so the unsaved-changes prompt runs before the swap.
    context.requestedOpenScene = path;
}

void ProjectPanel::InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(EditorCommands::InstantiatePrefab(path), context);
}

void ProjectPanel::DrawDetails(EditorContext& context) {
    if (ImGui::BeginChild("##project-details", ImVec2(0.0f, 0.0f), true)) {
        if (!m_Browser.HasSelection()) {
            ImGui::TextDisabled("Select a file to see details.");
        } else {
            const std::filesystem::path& path = m_Browser.Selected();
            std::error_code ec;
            const bool isDir = std::filesystem::is_directory(path, ec);
            ImGui::TextUnformatted(path.filename().string().c_str());
            if (isDir) {
                ImGui::TextDisabled("Folder");
                if (ImGui::Button("New Folder...")) {
                    RequestModal(Modal::CreateFolder, path);
                }
            } else {
                const EditorAssetPreview::Preview preview = EditorAssetPreview::Describe(path);
                ImGui::TextDisabled("%s  -  %s", preview.typeLabel.c_str(), preview.sizeLabel.c_str());

                // Asset database record (id, type, paths, cook status).
                if (const AssetMetadata* meta = FindAssetMeta(context, path)) {
                    ImGui::Separator();
                    ImGui::Text("Asset: %s",
                                meta->name.empty() ? path.filename().string().c_str() : meta->name.c_str());
                    ImGui::TextDisabled("Type: %s", AssetTypeToString(meta->type).c_str());
                    ImGui::TextDisabled("ID: %s", meta->id.ToString().c_str());
                    ImGui::TextDisabled(
                        "Cooked: %s", meta->cookedPath.empty() ? "(none)" : meta->cookedPath.generic_string().c_str());
                    ImGui::TextDisabled("Status: %s", AssetStatusLabel(*meta));
                    if (!meta->dependencies.empty()) {
                        ImGui::TextDisabled("Dependencies: %zu", meta->dependencies.size());
                    }
                }

                if (FileTypeRegistry::Classify(path).type == EditorFileType::Material) {
                    DrawMaterialEditor(context, path);
                } else if (preview.hasSnippet) {
                    ImGui::Separator();
                    ImGui::PushTextWrapPos(0.0f);
                    ImGui::TextDisabled("%s", preview.snippet.c_str());
                    ImGui::PopTextWrapPos();
                }
            }
        }
    }
    ImGui::EndChild();
}

void ProjectPanel::DrawModals(EditorContext& context) {
    (void)context;
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
            if (const Status status = m_Browser.RenameEntry(m_ModalTarget, m_NameBuffer); !status) {
                m_Status = status.error;
            } else {
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
        ImGui::TextDisabled("This permanently removes the file/folder from disk.");
        ImGui::Separator();
        if (ImGui::Button("Delete")) {
            if (const Status status = m_Browser.DeleteEntry(m_ModalTarget); !status) {
                m_Status = status.error;
            } else {
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
