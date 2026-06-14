#pragma once

namespace Hockey {

// Which manipulation the viewport gizmo performs. None disables the gizmo so
// clicks fall through to picking. Kept in its own lightweight header so the
// EditorContext and the transform tools can share it without pulling in
// ImGuizmo/ImGui.
enum class GizmoOperation {
    None,
    Translate,
    Rotate,
    Scale,
};

} // namespace Hockey
