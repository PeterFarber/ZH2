#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/Gizmos/GizmoOperation.hpp"

namespace Hockey {

class EditorContext;

// Wraps ImGuizmo for the current transform selection. The viewport panel
// supplies camera matrices and the screen rect of the rendered image. Single
// target edits use TransformEntity; grouped translate edits use one
// TransformEntities command so undo/redo restores the whole selection together.
class TransformGizmo {
public:
    // Returns true while the gizmo is hovered or actively used, so the viewport
    // suppresses click-to-pick. Screen rect is in ImGui screen coordinates.
    bool Manipulate(EditorContext& context, GizmoOperation operation, const glm::mat4& view,
                    const glm::mat4& projection, float rectX, float rectY, float rectWidth, float rectHeight);

private:
    bool m_Using = false;
    UUID m_Entity{0};
    TransformData m_BeforeLocal; // local TRS captured when manipulation started
    bool m_GroupTranslate = false;
    glm::mat4 m_PreviousPrimaryWorld{1.0f};
    std::vector<UUID> m_GroupEntities;
    std::vector<EntityTransformSnapshot> m_GroupSnapshots;
};

} // namespace Hockey
