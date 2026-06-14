#pragma once

#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Renderer/scene statistics. Populated with real metrics in a later step.
class StatsPanel : public Panel {
public:
    StatsPanel();
    void OnImGui(EditorContext& context) override;
};

} // namespace Hockey
