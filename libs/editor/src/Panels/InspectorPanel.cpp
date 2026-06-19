#include "Hockey/Editor/Panels/InspectorPanel.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>

#include <imgui.h>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

namespace {

constexpr const char* kDefaultTags[] = {"Untagged", "Player", "Puck", "Goal", "Spawn", "Rink", "Camera"};
constexpr const char* kDefaultLayers[] = {"Default", "Gameplay", "Physics", "Trigger", "UI", "Ignore Raycast"};

void DrawObjectIcon(const UUID uuid, float size) {
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##objectIcon", ImVec2(size, size));

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 border = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 fill = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 line = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImVec2 end(pos.x + size, pos.y + size);
    draw->AddRectFilled(pos, end, fill, 3.0f);
    draw->AddRect(pos, end, border, 3.0f);
    draw->AddLine(ImVec2(pos.x + size * 0.28f, pos.y + size * 0.38f),
                  ImVec2(pos.x + size * 0.5f, pos.y + size * 0.25f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.25f),
                  ImVec2(pos.x + size * 0.72f, pos.y + size * 0.38f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.28f, pos.y + size * 0.38f),
                  ImVec2(pos.x + size * 0.28f, pos.y + size * 0.65f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.72f, pos.y + size * 0.38f),
                  ImVec2(pos.x + size * 0.72f, pos.y + size * 0.65f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.28f, pos.y + size * 0.65f),
                  ImVec2(pos.x + size * 0.5f, pos.y + size * 0.78f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.72f, pos.y + size * 0.65f),
                  ImVec2(pos.x + size * 0.5f, pos.y + size * 0.78f), line, 1.5f);
    draw->AddLine(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.25f),
                  ImVec2(pos.x + size * 0.5f, pos.y + size * 0.78f), line, 1.0f);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("UUID: %s", uuid.ToString().c_str());
    }
}

template <std::size_t Count>
bool DrawPresetCombo(const char* id, std::string& value, const char* const (&presets)[Count], char* customBuffer,
                     std::size_t customBufferSize) {
    bool changed = false;
    const char* preview = value.empty() ? "(none)" : value.c_str();
    if (ImGui::BeginCombo(id, preview)) {
        if (ImGui::IsWindowAppearing()) {
            std::snprintf(customBuffer, customBufferSize, "%s", value.c_str());
        }

        for (const char* preset : presets) {
            const bool selected = value == preset;
            if (ImGui::Selectable(preset, selected)) {
                value = preset;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::Separator();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputTextWithHint("##custom", "Custom...", customBuffer, customBufferSize,
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (value != customBuffer) {
                value = customBuffer;
                changed = true;
            }
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && value != customBuffer) {
            value = customBuffer;
            changed = true;
        }

        ImGui::EndCombo();
    }
    return changed;
}

void ExecuteObjectSettingsEdit(EditorContext& context, Entity& entity, const char* label, const std::string& before) {
    Scene* scene = context.activeScene;
    if (scene == nullptr) {
        return;
    }
    const UUID uuid = entity.GetUUID();
    const std::string after = EntitySnapshot::CaptureEntity(*scene, uuid);
    if (!before.empty() && after != before) {
        context.undoRedo.Execute(EditorCommands::EditComponentField(uuid, label, before, after), context);
    }
}

} // namespace

InspectorPanel::InspectorPanel() : Panel(EditorPanelNames::kInspector) {}

void InspectorPanel::OnImGui(EditorContext& context) {
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    Scene* scene = context.activeScene;
    if (scene == nullptr) {
        ImGui::TextUnformatted("No active scene.");
        EndWindow();
        return;
    }

    context.selection.Validate(*scene);
    Entity entity = scene->FindEntityByUUID(context.selection.Primary());
    if (!entity) {
        ImGui::TextUnformatted("No entity selected.");
        EndWindow();
        return;
    }

    if (context.selection.Count() > 1) {
        ImGui::TextDisabled("%zu entities selected (editing primary)", context.selection.Count());
        ImGui::Separator();
    }

    DrawHeader(context, entity);
    ImGui::Separator();
    m_Inspector.Draw(context, entity);
    ImGui::Separator();
    m_AddMenu.Draw(context, entity);

    EndWindow();
}

void InspectorPanel::DrawHeader(EditorContext& context, Entity& entity) {
    const UUID uuid = entity.GetUUID();
    Scene* scene = context.activeScene;
    if (scene == nullptr) {
        return;
    }
    if (!entity.HasComponent<ObjectSettingsComponent>()) {
        entity.AddOrReplaceComponent<ObjectSettingsComponent>();
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    constexpr float kIconSize = 28.0f;

    DrawObjectIcon(uuid, kIconSize);
    ImGui::SameLine();

    ImGui::BeginGroup();
    bool active = entity.IsActive();
    if (ImGui::Checkbox("##active", &active)) {
        context.undoRedo.Execute(EditorCommands::SetActive(uuid, !active, active), context);
    }

    ImGui::SameLine();
    // Only refresh the buffer from the entity while not mid-edit, otherwise each
    // frame would clobber the user's in-progress typing (edits commit on enter /
    // focus loss as a single undo step).
    if (!m_NameEditing) {
        std::snprintf(m_NameBuffer, sizeof(m_NameBuffer), "%s", entity.GetName().c_str());
    }
    const float staticWidth =
        ImGui::CalcTextSize("Static").x + ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + style.FramePadding.x;
    const float nameWidth = ImGui::GetContentRegionAvail().x - staticWidth - style.ItemSpacing.x;
    ImGui::SetNextItemWidth(nameWidth > 96.0f ? nameWidth : 96.0f);
    ImGui::InputText("##name", m_NameBuffer, sizeof(m_NameBuffer));
    if (ImGui::IsItemActivated()) {
        m_NameEditing = true;
        m_NameOriginal = entity.GetName();
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (m_NameOriginal != m_NameBuffer) {
            context.undoRedo.Execute(EditorCommands::RenameEntity(uuid, m_NameOriginal, m_NameBuffer), context);
        }
    }
    if (ImGui::IsItemDeactivated()) {
        m_NameEditing = false;
    }

    ImGui::SameLine();
    bool isStatic = entity.GetComponent<ObjectSettingsComponent>().isStatic;
    if (ImGui::Checkbox("Static", &isStatic)) {
        const std::string before = EntitySnapshot::CaptureEntity(*scene, uuid);
        entity.GetComponent<ObjectSettingsComponent>().isStatic = isStatic;
        ExecuteObjectSettingsEdit(context, entity, "Static", before);
    }

    ImGui::Spacing();

    ObjectSettingsComponent& settings = entity.GetComponent<ObjectSettingsComponent>();
    const float labelWidth = ImGui::CalcTextSize("Layer").x;
    const float spacing = style.ItemSpacing.x;
    const float available = ImGui::GetContentRegionAvail().x;
    const float controlWidth = (available - labelWidth * 2.0f - spacing * 5.0f) * 0.5f;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Tag");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(controlWidth > 72.0f ? controlWidth : 72.0f);
    std::string tag = settings.tag;
    if (DrawPresetCombo("##tag", tag, kDefaultTags, m_TagBuffer, sizeof(m_TagBuffer))) {
        const std::string before = EntitySnapshot::CaptureEntity(*scene, uuid);
        settings.tag = std::move(tag);
        ExecuteObjectSettingsEdit(context, entity, "Tag", before);
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Layer");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    std::string layer = settings.layer;
    if (DrawPresetCombo("##layer", layer, kDefaultLayers, m_LayerBuffer, sizeof(m_LayerBuffer))) {
        const std::string before = EntitySnapshot::CaptureEntity(*scene, uuid);
        settings.layer = std::move(layer);
        ExecuteObjectSettingsEdit(context, entity, "Layer", before);
    }
    ImGui::EndGroup();
}

} // namespace Hockey
