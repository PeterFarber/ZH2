#pragma once

#include <glm/glm.hpp>

#include "Hockey/Renderer/Camera.hpp"

namespace Hockey {

struct EditorSettings;

// Free-fly / orbit editor camera for the scene viewport. Produces the Vulkan
// render data used to draw the offscreen target, plus a matching standard
// (non Y-flipped) projection consumed by ImGuizmo and world-space picking. The
// camera is pure math; the viewport panel feeds it input gathered from Dear
// ImGui IO each frame.
class EditorCamera {
public:
    // Per-frame input snapshot. The viewport panel decides when navigation is
    // allowed (e.g. only while the viewport is hovered); the camera just applies
    // whatever is set here.
    struct Input {
        bool hovered = false;       // mouse over the viewport image (gates wheel)
        glm::vec2 mouseDelta{0.0f}; // pixels since last frame
        float wheel = 0.0f;         // mouse wheel ticks this frame
        bool look = false;          // RMB held: mouse-look + WASD/QE fly
        bool pan = false;           // MMB held: screen-space pan
        bool orbit = false;         // Alt+LMB held: orbit around the pivot
        bool forward = false;
        bool back = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool fast = false; // Shift: fast-move multiplier
    };

    void Update(const Input& input, float deltaTime, const EditorSettings& settings);

    // Frames a world-space sphere (center + radius) so it fits in view (F key).
    void Focus(const glm::vec3& target, float radius);

    // Vulkan render data (perspective with the clip-space Y flip the renderer
    // expects). Used for RenderSceneToTarget.
    CameraRenderData RenderData(float aspectRatio) const;

    glm::mat4 ViewMatrix() const;
    // Standard right-handed projection WITHOUT the Vulkan Y flip, matching the
    // convention ImGuizmo and the picking unprojection use.
    glm::mat4 GizmoProjection(float aspectRatio) const;

    // World-space pick ray for a normalized viewport coordinate (x,y in [0,1]
    // with the origin at the top-left of the image). Returns a unit direction.
    void Ray(const glm::vec2& viewportUV, float aspectRatio, glm::vec3& outOrigin, glm::vec3& outDirection) const;

    glm::vec3 Position() const {
        return m_Position;
    }
    glm::vec3 Forward() const;
    glm::vec3 Right() const;
    glm::vec3 Up() const;

private:
    glm::vec3 m_Position{8.0f, 6.0f, 12.0f};
    float m_Yaw = -120.0f;  // degrees; -90 looks toward -Z
    float m_Pitch = -22.0f; // degrees; negative looks down
    float m_Fov = 60.0f;    // vertical FOV degrees
    float m_Near = 0.05f;
    float m_Far = 3000.0f;
    // Distance to the orbit/pan pivot in front of the camera; also the framing
    // distance used by Focus. Keeps pan speed and orbit radius sensible.
    float m_PivotDistance = 14.0f;
};

} // namespace Hockey
