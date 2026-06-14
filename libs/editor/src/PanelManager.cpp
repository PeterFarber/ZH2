#include "Hockey/Editor/PanelManager.hpp"

namespace Hockey {

Panel* PanelManager::FindPanel(const std::string& name) {
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        if (panel->GetName() == name) {
            return panel.get();
        }
    }
    return nullptr;
}

void PanelManager::OnUpdate(EditorContext& context, float deltaTime) {
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        if (panel->IsOpen()) {
            panel->OnUpdate(context, deltaTime);
        }
    }
}

void PanelManager::OnImGui(EditorContext& context) {
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        if (panel->IsOpen()) {
            panel->OnImGui(context);
        }
    }
}

void PanelManager::SetPanelOpen(const std::string& name, bool open) {
    if (Panel* panel = FindPanel(name)) {
        panel->SetOpen(open);
    }
}

} // namespace Hockey
