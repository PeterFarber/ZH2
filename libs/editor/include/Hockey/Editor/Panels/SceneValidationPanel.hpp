#pragma once

#include <vector>

#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Runs the ECS SceneValidator on the active scene and lists the resulting
// warnings/errors. Clicking an issue selects the offending entity. Validation
// is button-driven here; scene-load/before-save hooks are added with scene
// management.
class SceneValidationPanel : public Panel {
public:
    SceneValidationPanel();
    void OnImGui(EditorContext& context) override;

    // Re-runs validation against the given context's active scene.
    void Validate(EditorContext& context);

private:
    std::vector<SceneValidationIssue> m_Issues;
    bool m_HasRun = false;
    bool m_AutoValidate = false;
};

} // namespace Hockey
