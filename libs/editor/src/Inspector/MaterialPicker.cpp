#include "Hockey/Editor/Inspector/MaterialPicker.hpp"

#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Renderer/Material.hpp"

namespace Hockey::MaterialPicker {

namespace {

constexpr const char* kBuiltInPrefix = "BuiltIn.";
constexpr std::size_t kBuiltInPrefixLen = 8; // strlen("BuiltIn.")

// Debug/error helpers that should not appear as authoring choices.
bool IsHiddenBuiltIn(BuiltInMaterial material) {
    return material == BuiltInMaterial::ErrorMagenta || material == BuiltInMaterial::DebugCollider ||
           material == BuiltInMaterial::DebugTrigger;
}

std::string AssetDisplayName(const AssetMetadata& meta) {
    return !meta.name.empty() ? meta.name : meta.rawPath.filename().string();
}

// Short, human-readable label for the currently assigned material.
std::string CurrentLabel(std::uint64_t materialAsset, const std::string& materialName, AssetManager* assetManager) {
    if (materialAsset != 0) {
        if (assetManager != nullptr) {
            if (const AssetMetadata* meta = assetManager->Database().Find(AssetID{materialAsset})) {
                return AssetDisplayName(*meta);
            }
        }
        return "(missing asset)";
    }
    if (!materialName.empty()) {
        if (materialName.rfind(kBuiltInPrefix, 0) == 0) {
            return materialName.substr(kBuiltInPrefixLen);
        }
        return materialName;
    }
    return "(default)";
}

} // namespace

Result Draw(const char* label, std::uint64_t& materialAsset, std::string& materialName, AssetManager* assetManager) {
    Result result;

    const std::string current = CurrentLabel(materialAsset, materialName, assetManager);
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float comboWidth = ImGui::CalcItemWidth();

    ImGui::SetNextItemWidth(comboWidth > 0.0f ? comboWidth : 0.0f);
    if (ImGui::BeginCombo("##material", current.c_str())) {
        if (ImGui::Selectable("(default)", materialAsset == 0 && materialName.empty())) {
            materialAsset = 0;
            materialName.clear();
            result.changed = true;
            result.committed = true;
        }

        ImGui::Separator();
        ImGui::TextDisabled("Built-in");
        for (int i = 0; i < kBuiltInMaterialCount; ++i) {
            const auto material = static_cast<BuiltInMaterial>(i);
            if (IsHiddenBuiltIn(material)) {
                continue;
            }
            const char* name = BuiltInMaterialName(material);
            const std::string ref = std::string(kBuiltInPrefix) + name;
            const bool selected = materialAsset == 0 && materialName == ref;
            if (ImGui::Selectable(name, selected)) {
                materialName = ref;
                materialAsset = 0;
                result.changed = true;
                result.committed = true;
            }
        }

        if (assetManager != nullptr) {
            const std::vector<AssetMetadata*> materials = assetManager->Database().FindByType(AssetType::Material);
            if (!materials.empty()) {
                ImGui::Separator();
                ImGui::TextDisabled("Assets");
                for (const AssetMetadata* meta : materials) {
                    if (meta == nullptr) {
                        continue;
                    }
                    const bool selected = materialAsset == meta->id.Value();
                    ImGui::PushID(static_cast<const void*>(meta));
                    if (ImGui::Selectable(AssetDisplayName(*meta).c_str(), selected)) {
                        materialAsset = meta->id.Value();
                        materialName.clear();
                        result.changed = true;
                        result.committed = true;
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::EndCombo();
    }

    // Preserve the existing drag-drop workflow: drop a material asset from the
    // project tree directly onto the combo.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropType)) {
            const auto* dropped = static_cast<const AssetDragPayload*>(payload->Data);
            if (static_cast<AssetType>(dropped->type) == AssetType::Material && dropped->id != materialAsset) {
                materialAsset = dropped->id;
                materialName.clear();
                result.changed = true;
                result.committed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine(0.0f, spacing);
    ImGui::TextUnformatted(label);
    return result;
}

} // namespace Hockey::MaterialPicker
