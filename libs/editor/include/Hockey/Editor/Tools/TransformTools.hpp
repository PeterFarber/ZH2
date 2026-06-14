#pragma once

#include "Hockey/Editor/Gizmos/GizmoOperation.hpp"
#include "Hockey/Editor/Tools/EditorTool.hpp"

namespace Hockey {

// Persistent transform tools. Activating one sets the viewport gizmo mode in the
// EditorContext; the SceneViewportPanel reads it to drive ImGuizmo.
class TransformTool : public EditorTool {
public:
    explicit TransformTool(const char* name, const char* shortcut, GizmoOperation operation)
        : m_Name(name), m_Shortcut(shortcut), m_Operation(operation) {}

    const char* Name() const override {
        return m_Name;
    }
    const char* Category() const override {
        return "Transform";
    }
    const char* Shortcut() const override {
        return m_Shortcut;
    }

    void OnSelected(EditorContext& context) override;

private:
    const char* m_Name;
    const char* m_Shortcut;
    GizmoOperation m_Operation;
};

class SelectTool : public TransformTool {
public:
    SelectTool() : TransformTool("Select", "Q", GizmoOperation::None) {}
};

class MoveTool : public TransformTool {
public:
    MoveTool() : TransformTool("Move", "W", GizmoOperation::Translate) {}
};

class RotateTool : public TransformTool {
public:
    RotateTool() : TransformTool("Rotate", "E", GizmoOperation::Rotate) {}
};

class ScaleTool : public TransformTool {
public:
    ScaleTool() : TransformTool("Scale", "R", GizmoOperation::Scale) {}
};

} // namespace Hockey
