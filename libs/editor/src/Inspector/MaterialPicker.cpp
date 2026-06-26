#include "Hockey/Editor/Inspector/MaterialPicker.hpp"

#include <string>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"

namespace Hockey::MaterialPicker {

namespace {

std::string AssetDisplayName(const AssetMetadata& meta) {
    return !meta.name.empty() ? meta.name : meta.rawPath.filename().string();
}

// Short, human-readable label for the currently assigned material.
std::string CurrentLabel(std::uint64_t materialAsset, AssetManager* assetManager) {
    if (materialAsset != 0) {
        if (assetManager != nullptr) {
            if (const AssetMetadata* meta = assetManager->Database().Find(AssetID{materialAsset})) {
                return AssetDisplayName(*meta);
            }
        }
        return "(missing asset)";
    }
    return "(none)";
}

} // namespace

Result Draw(const char* label, std::uint64_t& materialAsset, AssetManager* assetManager) {
    Result result;

    const std::string current = CurrentLabel(materialAsset, assetManager);
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float comboWidth = ImGui::CalcItemWidth();

    ImGui::SetNextItemWidth(comboWidth > 0.0f ? comboWidth : 0.0f);
    if (ImGui::BeginCombo("##material", current.c_str())) {
        if (ImGui::Selectable("(none)", materialAsset == 0)) {
            materialAsset = 0;
            result.changed = true;
            result.committed = true;
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
