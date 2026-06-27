#include "Hockey/Editor/Inspector/FieldDrawers.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#include <glm/glm.hpp>
#include <imgui.h>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"

namespace Hockey::FieldDrawers {

void* FieldPointer(void* componentData, const FieldMetadata& field) {
    if (componentData == nullptr) {
        return nullptr;
    }
    return static_cast<char*>(componentData) + field.offset;
}


namespace {

bool DrawEnum(const char* label, const FieldMetadata& field, int* value) {
    int currentIndex = 0;
    for (std::size_t i = 0; i < field.enumValues.size(); ++i) {
        if (field.enumValues[i] == *value) {
            currentIndex = static_cast<int>(i);
            break;
        }
    }
    const char* preview =
        currentIndex < static_cast<int>(field.enumNames.size()) ? field.enumNames[currentIndex].c_str() : "";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview)) {
        for (std::size_t i = 0; i < field.enumNames.size(); ++i) {
            const bool selected = static_cast<int>(i) == currentIndex;
            if (ImGui::Selectable(field.enumNames[i].c_str(), selected) && i < field.enumValues.size()) {
                *value = field.enumValues[i];
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool HasFloatRange(const FieldMetadata& field) {
    return field.maxFloat > field.minFloat;
}

bool HasIntRange(const FieldMetadata& field) {
    return field.maxInt > field.minInt;
}

struct WidgetEdit {
    bool changed = false;
    bool started = false;
    bool committed = false;
};

WidgetEdit DrawColorControl(const char* label, float* value, int componentCount) {
    WidgetEdit edit;
    const ImGuiColorEditFlags flags =
        ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel;

    if (componentCount == 4) {
        edit.changed = ImGui::ColorEdit4(label, value, flags);
    } else {
        edit.changed = ImGui::ColorEdit3(label, value, flags);
    }
    edit.started = ImGui::IsItemActivated();
    edit.committed = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SameLine();
    if (EditorIconButton(EditorIcon::Settings, "Palette", "Open color palette",
                         ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
        ImGui::OpenPopup("##palette");
    }
    if (ImGui::IsItemActivated()) {
        edit.started = true;
    }

    if (ImGui::BeginPopup("##palette")) {
        const bool pickerChanged =
            componentCount == 4 ? ImGui::ColorPicker4("##picker", value, flags)
                                : ImGui::ColorPicker3("##picker", value, flags);
        if (pickerChanged) {
            edit.changed = true;
        }
        edit.started = edit.started || ImGui::IsItemActivated();
        edit.committed = edit.committed || ImGui::IsItemDeactivatedAfterEdit();

        ImGui::Separator();
        constexpr ImVec4 kSwatches[] = {
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 0.92f, 0.72f, 1.0f),
            ImVec4(0.58f, 0.74f, 1.0f, 1.0f), ImVec4(0.64f, 1.0f, 0.78f, 1.0f),
            ImVec4(1.0f, 0.56f, 0.52f, 1.0f), ImVec4(0.64f, 0.52f, 1.0f, 1.0f),
            ImVec4(1.0f, 0.7f, 0.35f, 1.0f),  ImVec4(0.08f, 0.1f, 0.12f, 1.0f),
        };
        for (int i = 0; i < static_cast<int>(sizeof(kSwatches) / sizeof(kSwatches[0])); ++i) {
            ImGui::PushID(i);
            if (i > 0 && i % 4 != 0) {
                ImGui::SameLine();
            }
            if (ImGui::ColorButton("##swatch", kSwatches[i], ImGuiColorEditFlags_NoTooltip,
                                   ImVec2(24.0f, 24.0f))) {
                value[0] = kSwatches[i].x;
                value[1] = kSwatches[i].y;
                value[2] = kSwatches[i].z;
                if (componentCount == 4) {
                    value[3] = kSwatches[i].w;
                }
                edit.changed = true;
                edit.started = true;
                edit.committed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }

    return edit;
}

// Draws an asset-reference slot: a labelled button showing the current asset's
// name (or "(none)"/"(missing)"), a drag-drop target that accepts a compatible
// asset payload, and a clear button. Returns true when the id changed.
bool DrawAssetRef(const FieldMetadata& field, std::uint64_t* id, AssetManager* assetManager) {
    bool changed = false;
    ImGui::PushID(field.name.c_str());

    const AssetType expected =
        field.assetTypeName.empty() ? AssetType::Unknown : AssetTypeFromString(field.assetTypeName);

    // Resolve a human-readable label for the current id.
    std::string label = "(none)";
    if (*id != 0) {
        label = "(missing)";
        if (assetManager != nullptr) {
            if (const AssetMetadata* meta = assetManager->Database().Find(AssetID{*id})) {
                label = !meta->name.empty() ? meta->name : meta->rawPath.filename().string();
            }
        } else {
            label = std::to_string(*id);
        }
    }

    // A point-and-click picker is only useful when we know the category and have
    // a database to enumerate; otherwise fall back to the button + drag-drop.
    const bool canPick = assetManager != nullptr && expected != AssetType::Unknown;

    const float clearWidth = ImGui::GetFrameHeight();
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float pickWidth = canPick ? clearWidth + spacing : 0.0f;
    const float buttonWidth = ImGui::CalcItemWidth() - clearWidth - spacing - pickWidth;
    ImGui::Button(label.c_str(), ImVec2(buttonWidth > 0.0f ? buttonWidth : 0.0f, 0.0f));

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropType)) {
            const auto* dropped = static_cast<const AssetDragPayload*>(payload->Data);
            const bool typeOk = expected == AssetType::Unknown || static_cast<AssetType>(dropped->type) == expected;
            if (typeOk && dropped->id != *id) {
                *id = dropped->id;
                changed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Dropdown that lists every asset of the expected category so a reference can
    // be assigned without dragging from the project tree.
    if (canPick) {
        ImGui::SameLine(0.0f, spacing);
        if (EditorIconButton(EditorIcon::Settings, "Asset Picker", "Open asset picker", ImVec2(clearWidth, 0.0f))) {
            ImGui::OpenPopup("##assetpick");
        }
        if (ImGui::BeginPopup("##assetpick")) {
            if (ImGui::Selectable("(none)", *id == 0) && *id != 0) {
                *id = 0;
                changed = true;
            }
            for (const AssetMetadata* meta : assetManager->Database().FindByType(expected)) {
                if (meta == nullptr) {
                    continue;
                }
                const std::string name = !meta->name.empty() ? meta->name : meta->rawPath.filename().string();
                ImGui::PushID(static_cast<const void*>(meta));
                if (ImGui::Selectable(name.c_str(), *id == meta->id.Value()) && *id != meta->id.Value()) {
                    *id = meta->id.Value();
                    changed = true;
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine(0.0f, spacing);
    if (EditorIconButton(EditorIcon::Clear, "Clear Asset", "Clear asset reference", ImVec2(clearWidth, 0.0f))) {
        if (*id != 0) {
            *id = 0;
            changed = true;
        }
    }
    ImGui::SameLine(0.0f, spacing);
    ImGui::TextUnformatted(field.displayName.c_str());

    ImGui::PopID();
    return changed;
}


} // namespace

FieldEdit Draw(const FieldMetadata& field, void* componentData, AssetManager* assetManager) {
    FieldEdit result;
    void* ptr = FieldPointer(componentData, field);
    if (ptr == nullptr) {
        return result;
    }

    const char* label = field.displayName.c_str();
    const float speed = field.speed > 0.0f ? field.speed : 0.1f;

    if (field.readOnly) {
        ImGui::BeginDisabled();
    }

    switch (field.type) {
    case FieldType::Bool:
        result.changed = ImGui::Checkbox(label, static_cast<bool*>(ptr));
        break;
    case FieldType::Int: {
        int* value = static_cast<int*>(ptr);
        const float intSpeed = field.speed > 0.0f ? field.speed : 1.0f;
        if (HasIntRange(field)) {
            result.changed = ImGui::SliderInt(label, value, field.minInt, field.maxInt);
        } else {
            result.changed = ImGui::DragInt(label, value, intSpeed, field.minInt, field.maxInt);
        }
        break;
    }
    case FieldType::Float:
        if (HasFloatRange(field)) {
            result.changed = ImGui::SliderFloat(label, static_cast<float*>(ptr), field.minFloat, field.maxFloat);
        } else {
            result.changed = ImGui::DragFloat(label, static_cast<float*>(ptr), speed, field.minFloat, field.maxFloat);
        }
        break;
    case FieldType::Vec2:
        if (HasFloatRange(field)) {
            result.changed = ImGui::SliderFloat2(label, static_cast<float*>(ptr), field.minFloat, field.maxFloat);
        } else {
            result.changed = ImGui::DragFloat2(label, static_cast<float*>(ptr), speed, field.minFloat, field.maxFloat);
        }
        break;
    case FieldType::Vec3: {
        if (field.hint == FieldHint::Color) {
            const WidgetEdit edit = DrawColorControl(label, static_cast<float*>(ptr), 3);
            result.changed = edit.changed;
            result.started = edit.started;
            result.committed = edit.committed;
        } else if (HasFloatRange(field)) {
            result.changed = ImGui::SliderFloat3(label, static_cast<float*>(ptr), field.minFloat, field.maxFloat);
        } else {
            result.changed = ImGui::DragFloat3(label, static_cast<float*>(ptr), speed, field.minFloat, field.maxFloat);
        }
        break;
    }
    case FieldType::Vec4: {
        if (field.hint == FieldHint::Color) {
            const WidgetEdit edit = DrawColorControl(label, static_cast<float*>(ptr), 4);
            result.changed = edit.changed;
            result.started = edit.started;
            result.committed = edit.committed;
        } else if (HasFloatRange(field)) {
            result.changed = ImGui::SliderFloat4(label, static_cast<float*>(ptr), field.minFloat, field.maxFloat);
        } else {
            result.changed = ImGui::DragFloat4(label, static_cast<float*>(ptr), speed, field.minFloat, field.maxFloat);
        }
        break;
    }
    case FieldType::Enum:
        result.changed = DrawEnum(label, field, static_cast<int*>(ptr));
        break;
    case FieldType::UUID: {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%s", static_cast<const UUID*>(ptr)->ToString().c_str());
        ImGui::InputText(label, buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
        break;
    }
    case FieldType::String: {
        auto* str = static_cast<std::string*>(ptr);
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), "%s", str->c_str());
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            *str = buffer;
            result.changed = true;
        }
        break;
    }
    case FieldType::Path: {
        auto* path = static_cast<std::filesystem::path*>(ptr);
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), "%s", path->string().c_str());
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            *path = std::filesystem::path(buffer);
            result.changed = true;
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropType)) {
                *path = std::filesystem::path(static_cast<const char*>(payload->Data));
                result.changed = true;
            }
            ImGui::EndDragDropTarget();
        }
        break;
    }
    case FieldType::AssetRef:
        result.changed = DrawAssetRef(field, static_cast<std::uint64_t*>(ptr), assetManager);
        break;
    }

    if (field.readOnly) {
        ImGui::EndDisabled();
        return FieldEdit{};
    }

    // Bracket the interaction so callers push a single undo step per edit.
    result.started = result.started || ImGui::IsItemActivated();
    result.committed = result.committed || ImGui::IsItemDeactivatedAfterEdit();
    // Discrete widgets (checkbox / enum combo / asset drop) commit the moment
    // they change rather than relying on a deactivation edge.
    if (result.changed &&
        (field.type == FieldType::Bool || field.type == FieldType::Enum || field.type == FieldType::AssetRef)) {
        result.started = true;
        result.committed = true;
    }
    return result;
}

} // namespace Hockey::FieldDrawers
