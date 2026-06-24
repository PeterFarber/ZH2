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

void PanelManager::ApplyPanelOpenStates(const EditorSettings& settings) {
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        panel->SetOpen(settings.PanelOpenOrDefault(panel->GetName(), panel->IsOpenByDefault()));
    }
}

std::vector<EditorPanelOpenState> PanelManager::CapturePanelOpenStates() const {
    std::vector<EditorPanelOpenState> states;
    states.reserve(m_Panels.size());
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        states.push_back(EditorPanelOpenState{panel->GetName(), panel->IsOpen()});
    }
    return states;
}

void PanelManager::ResetPanelOpenStates() {
    for (const std::unique_ptr<Panel>& panel : m_Panels) {
        panel->ResetOpenState();
    }
}

} // namespace Hockey
