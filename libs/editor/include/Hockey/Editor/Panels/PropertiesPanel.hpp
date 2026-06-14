#pragma once

#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Secondary properties panel (project/scene settings). Hidden by default.
class PropertiesPanel : public Panel {
public:
    PropertiesPanel();
    void OnImGui(EditorContext& context) override;
};

} // namespace Hockey
