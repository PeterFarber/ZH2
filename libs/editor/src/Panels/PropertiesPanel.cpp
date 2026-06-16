#include "Hockey/Editor/Panels/PropertiesPanel.hpp"

#include <cstring>
#include <string>
#include <string_view>

#include <imgui.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneMode.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

PropertiesPanel::PropertiesPanel() : Panel(EditorPanelNames::kProperties, /*openByDefault=*/false) {}

void PropertiesPanel::OnImGui(EditorContext& context) {
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

    ImGui::SeparatorText("Scene");

    char nameBuffer[256] = {};
    const std::string& name = scene->GetName();
    std::strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        scene->SetName(nameBuffer);
        context.MarkDirty();
    }

    const std::string_view mode = SceneModeToString(scene->GetMode());
    ImGui::Text("Mode: %.*s", static_cast<int>(mode.size()), mode.data());
    ImGui::Text("Entities: %zu", scene->EntityCount());
    ImGui::Text("Systems: %zu", scene->SystemCount());

    if (context.HasActiveScenePath()) {
        ImGui::Spacing();
        ImGui::SeparatorText("File");
        ImGui::TextWrapped("%s", context.activeScenePath.string().c_str());
    }

    EndWindow();
}

} // namespace Hockey
