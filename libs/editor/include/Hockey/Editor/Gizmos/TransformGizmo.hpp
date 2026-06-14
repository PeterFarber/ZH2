#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/Gizmos/GizmoOperation.hpp"

namespace Hockey {

class EditorContext;

// Wraps ImGuizmo for the primary selection. The viewport panel supplies the
// camera matrices and the screen rect of the rendered image; the gizmo edits the
// entity's world transform live and pushes a single TransformEntity command to
// the undo stack when a manipulation completes.
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
};

} // namespace Hockey
