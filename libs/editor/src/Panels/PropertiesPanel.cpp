#include "Hockey/Editor/Panels/PropertiesPanel.hpp"

#include <imgui.h>

#include "Hockey/Editor/Dockspace.hpp"

namespace Hockey {

PropertiesPanel::PropertiesPanel() : Panel(EditorPanelNames::kProperties, /*openByDefault=*/false) {}

void PropertiesPanel::OnImGui(EditorContext& /*context*/) {
    if (BeginWindow()) {
        ImGui::TextUnformatted("Project/scene properties will appear here.");
    }
    EndWindow();
}

} // namespace Hockey
