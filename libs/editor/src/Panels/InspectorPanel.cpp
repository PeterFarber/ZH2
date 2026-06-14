#include "Hockey/Editor/Panels/InspectorPanel.hpp"

#include <cstdio>

#include <imgui.h>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

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
    ImGui::SetNextItemWidth(-1.0f);
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

    ImGui::TextDisabled("UUID: %s", uuid.ToString().c_str());
}

} // namespace Hockey
