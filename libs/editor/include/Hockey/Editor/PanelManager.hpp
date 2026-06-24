#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

class EditorContext;

// Owns the editor's panels and drives their per-frame update/draw. Panels are
// created once at startup via AddPanel and live for the editor's lifetime.
class PanelManager {
public:
    template <typename T, typename... Args> T& AddPanel(Args&&... args) {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *panel;
        m_Panels.push_back(std::move(panel));
        return ref;
    }

    Panel* FindPanel(const std::string& name);

    void OnUpdate(EditorContext& context, float deltaTime);
    void OnImGui(EditorContext& context);

    void SetPanelOpen(const std::string& name, bool open);
    void RequestPanelFocus(EditorContext& context, const std::string& name);
    void ApplyPanelOpenStates(const EditorSettings& settings);
    std::vector<EditorPanelOpenState> CapturePanelOpenStates() const;
    void ResetPanelOpenStates();

    // Panels in registration order; used by the Window menu to list toggles.
    const std::vector<std::unique_ptr<Panel>>& Panels() const {
        return m_Panels;
    }

private:
    std::vector<std::unique_ptr<Panel>> m_Panels;
};

} // namespace Hockey
