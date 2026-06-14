#pragma once

#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Physics preview + visualization controls: enable preview, play/pause/step,
// reset, and the collider/trigger/body-centre/contact overlay toggles. The
// preview simulation itself is owned by EditorApp (EditorPhysicsPreview).
class PhysicsPanel : public Panel {
public:
    PhysicsPanel();
    void OnImGui(EditorContext& context) override;
};

} // namespace Hockey
