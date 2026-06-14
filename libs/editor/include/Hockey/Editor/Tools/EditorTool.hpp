#pragma once

namespace Hockey {

class EditorContext;

// Base class for an editor tool. Two flavours exist:
//   - Persistent tools (Select/Move/Rotate/Scale) stay active and drive the
//     viewport gizmo; OnSelected/OnDeselected bracket their active lifetime.
//   - Instant tools (placement tools) perform a one-shot action on activation
//     (OnSelected) and never become the persistent active tool.
class EditorTool {
public:
    virtual ~EditorTool();

    virtual const char* Name() const = 0;

    // Toolbar/menu grouping label, e.g. "Transform" / "Create" / "Hockey".
    virtual const char* Category() const {
        return "General";
    }

    // Optional keyboard shortcut shown in menus (display only).
    virtual const char* Shortcut() const {
        return nullptr;
    }

    // Instant tools spawn/act on activation and are not retained as active.
    virtual bool IsInstant() const {
        return false;
    }

    virtual void OnSelected(EditorContext& context);
    virtual void OnDeselected(EditorContext& context);
    virtual void OnUpdate(EditorContext& context, float deltaTime);
    virtual void OnViewportImGui(EditorContext& context);
};

} // namespace Hockey
