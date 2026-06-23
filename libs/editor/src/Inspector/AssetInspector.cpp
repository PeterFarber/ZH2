#include "Hockey/Editor/Inspector/AssetInspector.hpp"

#include <cstdio>
#include <filesystem>
#include <system_error>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/Project/EditorAssetPreview.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

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

} // namespace

void AssetInspector::Draw(EditorContext& context, AssetID assetId) {
    if (context.assetManager == nullptr) {
        ImGui::TextUnformatted("No asset database loaded.");
        return;
    }

    const AssetMetadata* meta = context.assetManager->Database().Find(assetId);
    if (meta == nullptr || !IsVisibleCookedAsset(*meta)) {
        ImGui::TextUnformatted("Selected asset is no longer available.");
        if (ImGui::Button("Clear Selection")) {
            context.ClearAssetSelection();
        }
        return;
    }

    DrawMetadata(context, *meta);
    if (meta->type == AssetType::Material) {
        ImGui::Separator();
        DrawMaterialEditor(context, ResolveProjectPath(meta->rawPath));
    } else {
        const EditorAssetPreview::Preview preview = EditorAssetPreview::Describe(ResolveProjectPath(meta->rawPath));
        if (preview.hasSnippet) {
            ImGui::Separator();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextDisabled("%s", preview.snippet.c_str());
            ImGui::PopTextWrapPos();
        }
    }

    if (!m_Status.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "%s", m_Status.c_str());
    }
}

void AssetInspector::DrawMetadata(EditorContext& context, const AssetMetadata& meta) {
    const std::filesystem::path raw = ResolveProjectPath(meta.rawPath);
    const std::filesystem::path cooked = ResolveProjectPath(meta.cookedPath);

    ImGui::TextUnformatted(meta.name.empty() ? raw.filename().string().c_str() : meta.name.c_str());
    ImGui::TextDisabled("Type: %s", AssetTypeToString(meta.type).c_str());
    ImGui::TextDisabled("ID: %s", meta.id.ToString().c_str());
    ImGui::TextDisabled("Raw: %s", ProjectRelativePath(raw).generic_string().c_str());
    ImGui::TextDisabled("Cooked: %s", ProjectRelativePath(cooked).generic_string().c_str());
    ImGui::TextDisabled("Status: %s", AssetStatusLabel(meta));
    if (!meta.dependencies.empty()) {
        ImGui::TextDisabled("Dependencies: %zu", meta.dependencies.size());
    }

    ImGui::Separator();
    if (meta.type == AssetType::Scene) {
        if (ImGui::Button("Open Scene")) {
            context.requestedOpenScene = raw;
        }
        EditorTooltip::ForLastItem("Open this scene asset in the editor.");
        ImGui::SameLine();
    }
    if (ImGui::Button("Reimport & Recook")) {
        ReimportAndRecook(context, meta);
    }
    EditorTooltip::ForLastItem("Reimport the raw source asset, recook dirty dependents, and save the database.");
    ImGui::SameLine();
    if (ImGui::Button("Recook")) {
        Recook(context, meta);
    }
    EditorTooltip::ForLastItem("Mark this cooked asset dirty and cook it immediately.");

    if (ImGui::Button("Copy Asset ID")) {
        ImGui::SetClipboardText(meta.id.ToString().c_str());
    }
    EditorTooltip::ForLastItem("Copy this cooked asset's stable ID to the clipboard.");
    ImGui::SameLine();
    if (ImGui::Button("Reveal Source")) {
        ProjectBrowser::Reveal(raw);
    }
    EditorTooltip::ForLastItem("Reveal the raw source file in the system file manager.");
    ImGui::SameLine();
    if (ImGui::Button("Reveal Cooked")) {
        ProjectBrowser::Reveal(cooked);
    }
    EditorTooltip::ForLastItem("Reveal the cooked output file in the system file manager.");
}

void AssetInspector::DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path) {
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
    EditorTooltip::ForLastItem("Material display name written to the source asset.");

    ImGui::SeparatorText("Surface");
    changed |= ImGui::ColorEdit4("Albedo", &m_EditMaterial.baseColor.x);
    EditorTooltip::ForLastItem("Base color and alpha.");
    changed |= ImGui::SliderFloat("Metallic", &m_EditMaterial.metallic, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Metallic surface response.");
    changed |= ImGui::SliderFloat("Roughness", &m_EditMaterial.roughness, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Surface microsurface roughness.");
    changed |= ImGui::SliderFloat("Normal Strength", &m_EditMaterial.normalStrength, 0.0f, 4.0f);
    EditorTooltip::ForLastItem("Strength applied to the normal map.");
    changed |= ImGui::SliderFloat("Occlusion", &m_EditMaterial.occlusionStrength, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Ambient occlusion texture strength.");
    changed |= ImGui::ColorEdit3("Emission", &m_EditMaterial.emissiveColor.x,
                                 ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
    EditorTooltip::ForLastItem("Emissive color contribution.");
    changed |= ImGui::SliderFloat("Emission Strength", &m_EditMaterial.emissiveStrength, 0.0f, 16.0f);
    EditorTooltip::ForLastItem("Emissive intensity multiplier.");

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
    EditorTooltip::ForLastItem("Opaque ignores alpha, Mask clips by cutoff, Blend uses alpha blending.");
    ImGui::BeginDisabled(m_EditMaterial.alphaMode != "Mask");
    changed |= ImGui::SliderFloat("Alpha Cutoff", &m_EditMaterial.alphaCutoff, 0.0f, 1.0f);
    EditorTooltip::ForLastItem("Alpha threshold used by Mask rendering mode.");
    ImGui::EndDisabled();

    ImGui::SeparatorText("Maps");
    ImGui::TextDisabled("Drag a texture asset from Project onto a slot.");

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
        EditorTooltip::ForLastItem("Drop a texture asset here to assign this map.");
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
            EditorTooltip::ForLastItem("Clear this texture slot.");
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
    EditorTooltip::ForLastItem("UV tiling applied to every texture map.");
    changed |= ImGui::DragFloat2("Offset", &m_EditMaterial.offset.x, 0.005f, -256.0f, 256.0f);
    EditorTooltip::ForLastItem("UV offset applied to every texture map.");

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
                context.SelectAsset(id);
            }
            m_MaterialPreviewActive = false;
            m_Status = "Saved material";
        } else {
            m_Status = "Saved material (no asset manager to cook)";
        }
    }
    EditorTooltip::ForLastItem("Save material edits, reimport, and cook dependent assets.");
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
    EditorTooltip::ForLastItem("Discard unsaved live preview edits and reload from disk.");
    ImGui::EndDisabled();
}

void AssetInspector::ApplyMaterialPreview(EditorContext& context) {
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

void AssetInspector::ReimportAndRecook(EditorContext& context, const AssetMetadata& meta) {
    if (context.assetManager == nullptr) {
        return;
    }
    const AssetID id = meta.id;
    const std::filesystem::path rawPath = ResolveProjectPath(meta.rawPath);
    const std::string name = meta.name;

    if (const Status status = context.assetManager->ImportAsset(rawPath); !status) {
        m_Status = status.error;
        return;
    }
    context.assetManager->Database().MarkDirtyWithDependents(id);
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Reimported and recooked " + name;
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
}

void AssetInspector::Recook(EditorContext& context, const AssetMetadata& meta) {
    if (context.assetManager == nullptr) {
        return;
    }
    const AssetID id = meta.id;
    const std::string name = meta.name;

    context.assetManager->Database().MarkDirtyWithDependents(id);
    if (const Status status = context.assetManager->CookAllDirty(); !status) {
        m_Status = status.error;
    } else {
        m_Status = "Recooked " + name;
    }
    context.assetManager->SaveDatabase();
    InvalidateAssetEvents(context);
}

void AssetInspector::InvalidateAssetEvents(EditorContext& context) {
    if (context.assetManager == nullptr || context.renderer == nullptr) {
        return;
    }
    const std::vector<AssetEvent>& events = context.assetManager->PollEvents();
    for (const AssetEvent& event : events) {
        context.renderer->InvalidateAsset(event.id.Value());
    }
}

} // namespace Hockey
