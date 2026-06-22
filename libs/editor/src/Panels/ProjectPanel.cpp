#include "Hockey/Editor/Panels/ProjectPanel.hpp"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
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
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
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
        DrawToolbar(context);
        ImGui::Separator();

        // Split the panel horizontally: file tree on the left, selected-asset
        // details on the right. Both panes use the full panel height.
        const float treeWidth = ImGui::GetContentRegionAvail().x * 0.4f;
        if (ImGui::BeginChild("##project-tree", ImVec2(treeWidth > 0.0f ? treeWidth : 0.0f, 0.0f), true)) {
            if (m_Browser.SearchActive()) {
                DrawSearchResults(context);
            } else {
                DrawTree(context);
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        DrawDetails(context);
        DrawModals(context);
    }
    EndWindow();
}

void ProjectPanel::DrawToolbar(EditorContext& context) {
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("##search", "Search...", m_Browser.SearchBuffer(), m_Browser.SearchBufferSize());
    EditorTooltip::ForLastItem("Filter project files by name");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        m_Browser.SearchBuffer()[0] = '\0';
    }
    EditorTooltip::ForLastItem("Clear the project search filter");
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        m_Browser.ClearSelectionIfMissing();
        m_Status.clear();
    }
    EditorTooltip::ForLastItem("Refresh the project browser selection state");
    if (context.assetManager != nullptr) {
        ImGui::SameLine();
        if (ImGui::Button("Scan")) {
            if (const Status status = context.assetManager->DiscoverRawAssets(); !status) {
                m_Status = status.error;
            } else {
                m_Status = "Scanned raw assets";
            }
        }
        EditorTooltip::ForLastItem("Scan raw asset folders for new or changed files");
        ImGui::SameLine();
        if (ImGui::Button("Import All")) {
            if (const Status status = context.assetManager->ImportAll(); !status) {
                m_Status = status.error;
            } else {
                m_Status = "Imported assets";
            }
        }
        EditorTooltip::ForLastItem("Import every discovered raw asset into the asset database");
        ImGui::SameLine();
        if (ImGui::Button("Cook Dirty")) {
            if (const Status status = context.assetManager->CookAllDirty(); !status) {
                m_Status = status.error;
            } else {
                m_Status = "Cooked dirty assets";
            }
        }
        EditorTooltip::ForLastItem("Cook assets marked dirty by imports or edits");
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
    if (!entry.type.supported) {
        const std::string tooltip = std::string(entry.type.label) + " (preview only, not imported)";
        EditorTooltip::ForLastItem(tooltip);
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
    EditorTooltip::ForLastItem(imported ? "Reimport this raw asset and update its metadata"
                                        : "Import this raw asset into the asset database");
    if (imported) {
        if (ImGui::MenuItem("Cook")) {
            if (const Status status = assets->CookAsset(meta->id); !status) {
                m_Status = status.error;
            } else {
                m_Status.clear();
            }
        }
        EditorTooltip::ForLastItem("Cook this asset using its current metadata");
    }
    if (imported) {
        if (ImGui::MenuItem("Recook")) {
            assets->Database().MarkDirty(meta->id);
            if (const Status status = assets->CookAsset(meta->id); !status) {
                m_Status = status.error;
            } else {
                m_Status.clear();
            }
        }
        EditorTooltip::ForLastItem("Mark this asset dirty and cook it immediately");
    }
    if (ImGui::MenuItem("Recook All Dirty")) {
        if (const Status status = assets->CookAllDirty(); !status) {
            m_Status = status.error;
        } else {
            m_Status.clear();
        }
        assets->SaveDatabase();
    }
    EditorTooltip::ForLastItem("Cook every asset currently marked dirty");
    if (imported) {
        if (ImGui::MenuItem("Copy Asset ID")) {
            ImGui::SetClipboardText(meta->id.ToString().c_str());
        }
        EditorTooltip::ForLastItem("Copy this asset's stable ID to the clipboard");
    }
    if (imported) {
        if (ImGui::MenuItem("Delete Metadata")) {
            const AssetID id = meta->id;
            if (const Status status = assets->DeleteMetadata(id); !status) {
                m_Status = status.error;
            } else {
                assets->SaveDatabase();
                if (context.renderer != nullptr) {
                    context.renderer->InvalidateAsset(id.Value());
                }
                m_Status = "Deleted metadata";
            }
        }
        EditorTooltip::ForLastItem("Remove this asset's metadata record without deleting the raw file");
    }
    if (ImGui::MenuItem("Validate References")) {
        std::vector<AssetValidationIssue> issues;
        const Status status = assets->ValidateReferences(&issues);
        m_Status = status ? "Validation passed (" + std::to_string(issues.size()) + " notes)"
                          : "Validation found issues: " + std::to_string(issues.size());
    }
    EditorTooltip::ForLastItem("Check asset database references for missing or stale IDs");
}

void ProjectPanel::DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path) {
    // Lazily (re)load the material whenever the selected file changes. Any unsaved
    // live preview on the previously edited material is discarded so the viewport
    // reflects what is actually on disk.
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

    // Transparency: Opaque ignores alpha, Mask does a hard cutoff at AlphaCutoff,
    // Blend does order-dependent alpha blending. AlphaCutoff only applies to Mask.
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

    // A texture slot: a thumbnail (drop target) + label + filename + clear. The
    // slot stores a project-relative raw texture path; a texture dropped from the
    // project tree (kAssetDragDropType) sets it, and save resolves it to an
    // AssetID via the cooker. Returns true when the slot changed.
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
            ImGui::Button("##empty", thumbSize); // empty drop box when no texture / no preview
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

    // UV transform shared by every map: tiling (scale) then offset.
    changed |= ImGui::DragFloat2("Tiling", &m_EditMaterial.tiling.x, 0.01f, 0.0f, 256.0f);
    EditorTooltip::ForLastItem("UV tiling applied to every texture map");
    changed |= ImGui::DragFloat2("Offset", &m_EditMaterial.offset.x, 0.005f, -256.0f, 256.0f);
    EditorTooltip::ForLastItem("UV offset applied to every texture map");

    // Live preview: push edits straight to the renderer (no disk write) so the
    // scene viewport updates as the user drags sliders / drops textures.
    if (changed) {
        ApplyMaterialPreview(context);
    }

    ImGui::Separator();
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
            m_MaterialPreviewActive = false; // saved state now matches disk
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
            context.renderer->InvalidateAsset(m_EditMaterialId); // drop preview, reload from disk
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
