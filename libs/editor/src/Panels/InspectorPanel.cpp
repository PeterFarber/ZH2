#include "Hockey/Editor/Panels/InspectorPanel.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include <imgui.h>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

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
    const char* glyph = EditorIconGlyph(EditorIcon::Prefab);
    if (glyph != nullptr && glyph[0] != '\0') {
        const ImVec2 glyphSize = ImGui::CalcTextSize(glyph);
        draw->AddText(ImVec2(pos.x + (size - glyphSize.x) * 0.5f, pos.y + (size - glyphSize.y) * 0.5f), line, glyph);
    } else {
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
    }

    const std::string tooltip = "Entity UUID: " + uuid.ToString();
    EditorTooltip::ForLastItem(std::string_view{tooltip});
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

    context.SyncAssetSelectionWithEntitySelection();

    DrawLockToggle(context);
    ImGui::Separator();

    if (m_InspectorLocked && m_LockedAssetId.IsValid()) {
        m_AssetInspector.Draw(context, m_LockedAssetId);
        EndWindow();
        return;
    }

    if (!m_InspectorLocked && context.SelectedAsset().IsValid()) {
        m_AssetInspector.Draw(context, context.SelectedAsset());
        EndWindow();
        return;
    }

    Scene* scene = context.activeScene;
    if (scene == nullptr) {
        ImGui::TextUnformatted("No active scene.");
        EndWindow();
        return;
    }

    if (!m_InspectorLocked) {
        context.selection.Validate(*scene);
    }

    const UUID inspectedEntityId = m_InspectorLocked ? m_LockedEntityId : context.selection.Primary();
    Entity entity = scene->FindEntityByUUID(inspectedEntityId);
    if (!entity) {
        ImGui::TextUnformatted(m_InspectorLocked ? "Locked entity is no longer available." : "No entity selected.");
        if (m_InspectorLocked && ImGui::Button("Clear Lock")) {
            m_InspectorLocked = false;
            m_LockedAssetId = {};
            m_LockedEntityId = UUID(0);
        }
        EndWindow();
        return;
    }

    if (!m_InspectorLocked && context.selection.Count() > 1) {
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

void InspectorPanel::DrawLockToggle(EditorContext& context) {
    const bool hasSelection = context.SelectedAsset().IsValid() || context.selection.Primary().IsValid();
    ImGui::BeginDisabled(!m_InspectorLocked && !hasSelection);
    if (EditorIconToggleButton(EditorIcon::Locked, "InspectorLock", m_InspectorLocked,
                               m_InspectorLocked ? "Unlock Inspector from the current item."
                                                 : "Lock Inspector to the current item.")) {
        if (m_InspectorLocked) {
            m_InspectorLocked = false;
            m_LockedAssetId = {};
            m_LockedEntityId = UUID(0);
        } else if (context.SelectedAsset().IsValid()) {
            m_InspectorLocked = true;
            m_LockedAssetId = context.SelectedAsset();
            m_LockedEntityId = UUID(0);
        } else if (context.selection.Primary().IsValid()) {
            m_InspectorLocked = true;
            m_LockedAssetId = {};
            m_LockedEntityId = context.selection.Primary();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled(m_InspectorLocked ? "Locked" : "Follow Selection");
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
    EditorTooltip::ForLastItem("Toggles whether this entity is active in the scene hierarchy.");

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
    EditorTooltip::ForLastItem("Renames the selected entity after editing finishes.");
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
    EditorTooltip::ForLastItem("Marks the entity as static editor data for later validation and systems.");

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
    const bool tagChanged = DrawPresetCombo("##tag", tag, kDefaultTags, m_TagBuffer, sizeof(m_TagBuffer));
    EditorTooltip::ForLastItem("Sets the entity tag used by editor tools and scene validation.");
    if (tagChanged) {
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
    const bool layerChanged = DrawPresetCombo("##layer", layer, kDefaultLayers, m_LayerBuffer, sizeof(m_LayerBuffer));
    EditorTooltip::ForLastItem("Sets the entity layer used by editor tools and future filtering.");
    if (layerChanged) {
        const std::string before = EntitySnapshot::CaptureEntity(*scene, uuid);
        settings.layer = std::move(layer);
        ExecuteObjectSettingsEdit(context, entity, "Layer", before);
    }
    ImGui::EndGroup();
}

} // namespace Hockey
