#include "Hockey/Editor/PanelManager.hpp"

#include <imgui.h>

#include "Hockey/Editor/EditorContext.hpp"

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
    if (!context.requestedPanelFocus.empty()) {
        if (Panel* panel = FindPanel(context.requestedPanelFocus)) {
            panel->SetOpen(true);
        } else {
            context.requestedPanelFocus.clear();
        }
    }

    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        if (panel->IsOpen()) {
            const bool focusRequested = panel->GetName() == context.requestedPanelFocus;
            if (focusRequested) {
                ImGui::SetNextWindowFocus();
            }
            panel->OnImGui(context);
            if (focusRequested) {
                context.requestedPanelFocus.clear();
            }
        }
    }
}

void PanelManager::SetPanelOpen(const std::string& name, bool open) {
    if (Panel* panel = FindPanel(name)) {
        panel->SetOpen(open);
    }
}

void PanelManager::RequestPanelFocus(EditorContext& context, const std::string& name) {
    if (Panel* panel = FindPanel(name)) {
        panel->SetOpen(true);
        context.requestedPanelFocus = name;
    }
}

} // namespace Hockey
