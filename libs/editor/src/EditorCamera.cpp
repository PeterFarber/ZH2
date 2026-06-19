#include "Hockey/Editor/EditorCamera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "Hockey/Editor/EditorSettings.hpp"

namespace Hockey {

namespace {
constexpr glm::vec3 kWorldUp{0.0f, 1.0f, 0.0f};

float SafeAspect(float aspect) {
    return aspect > 0.0f ? aspect : 1.0f;
}
} // namespace

glm::vec3 EditorCamera::Forward() const {
    const float cy = std::cos(glm::radians(m_Yaw));
    const float sy = std::sin(glm::radians(m_Yaw));
    const float cp = std::cos(glm::radians(m_Pitch));
    const float sp = std::sin(glm::radians(m_Pitch));
    return glm::normalize(glm::vec3(cy * cp, sp, sy * cp));
}

glm::vec3 EditorCamera::Right() const {
    return glm::normalize(glm::cross(Forward(), kWorldUp));
}

glm::vec3 EditorCamera::Up() const {
    return glm::normalize(glm::cross(Right(), Forward()));
}

glm::mat4 EditorCamera::ViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + Forward(), kWorldUp);
}

void EditorCamera::Update(const Input& input, float deltaTime, const EditorSettings& settings) {
    const float sensitivity = settings.cameraMouseSensitivity;

    if (input.look) {
        m_Yaw += input.mouseDelta.x * sensitivity;
        m_Pitch -= input.mouseDelta.y * sensitivity;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

        const float speed = settings.cameraMoveSpeed * (input.fast ? settings.cameraFastMultiplier : 1.0f) * deltaTime;
        glm::vec3 move{0.0f};
        if (input.forward) {
            move += Forward();
        }
        if (input.back) {
            move -= Forward();
        }
        if (input.right) {
            move += Right();
        }
        if (input.left) {
            move -= Right();
        }
        if (input.up) {
            move += kWorldUp;
        }
        if (input.down) {
            move -= kWorldUp;
        }
        if (glm::dot(move, move) > 0.0f) {
            m_Position += glm::normalize(move) * speed;
        }
    } else if (input.orbit) {
        // Rotate around the pivot directly in front of the camera, keeping the
        // pivot fixed so the camera swings around the focused point.
        const glm::vec3 pivot = m_Position + Forward() * m_PivotDistance;
        m_Yaw += input.mouseDelta.x * sensitivity;
        m_Pitch -= input.mouseDelta.y * sensitivity;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
        m_Position = pivot - Forward() * m_PivotDistance;
    } else if (input.pan) {
        const float panSpeed = std::max(m_PivotDistance, 1.0f) * 0.0015f;
        m_Position -= Right() * input.mouseDelta.x * panSpeed;
        m_Position += Up() * input.mouseDelta.y * panSpeed;
    }

    if (input.hovered && input.wheel != 0.0f) {
        // Dolly along the view direction; scale the step with the pivot distance
        // so zooming stays smooth far from and close to the focus point.
        const float step = input.wheel * (m_PivotDistance * 0.12f + 0.4f);
        m_Position += Forward() * step;
        m_PivotDistance = std::max(0.5f, m_PivotDistance - step);
    }
}

void EditorCamera::Focus(const glm::vec3& target, float radius) {
    const float halfFov = glm::radians(m_Fov) * 0.5f;
    const float distance = std::max(radius / std::max(std::tan(halfFov), 1e-3f), m_Near * 4.0f) * 1.2f;
    m_PivotDistance = distance;
    m_Position = target - Forward() * distance;
}

void EditorCamera::SetFromRenderData(const CameraRenderData& data) {
    const glm::mat4 world = glm::inverse(data.view);
    m_Position = glm::vec3(world[3]);

    const glm::vec3 forward = glm::normalize(-glm::vec3(world[2]));
    m_Pitch = glm::degrees(std::asin(std::clamp(forward.y, -1.0f, 1.0f)));
    m_Yaw = glm::degrees(std::atan2(forward.z, forward.x));

    const float projectionY = std::abs(data.projection[1][1]);
    if (projectionY > 1e-5f) {
        m_Fov = glm::degrees(2.0f * std::atan(1.0f / projectionY));
    }
    m_Near = std::max(data.nearClip, 1e-3f);
    m_Far = std::max(data.farClip, m_Near + 1e-2f);

    if (std::abs(forward.y) > 1e-4f) {
        const float groundT = -m_Position.y / forward.y;
        if (groundT > 0.5f) {
            m_PivotDistance = groundT;
            return;
        }
    }
    m_PivotDistance = std::max(m_PivotDistance, 1.0f);
}

void EditorCamera::SetViewDirection(const glm::vec3& forward) {
    if (glm::dot(forward, forward) < 1e-6f) {
        return;
    }

    const glm::vec3 pivot = m_Position + Forward() * m_PivotDistance;
    const glm::vec3 f = glm::normalize(forward);
    m_Pitch = glm::degrees(std::asin(std::clamp(f.y, -1.0f, 1.0f)));
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
    m_Yaw = glm::degrees(std::atan2(f.z, f.x));
    m_Position = pivot - Forward() * m_PivotDistance;
}

CameraRenderData EditorCamera::RenderData(float aspectRatio) const {
    CameraRenderData data;
    data.view = ViewMatrix();
    data.position = m_Position;
    data.nearClip = m_Near;
    data.farClip = m_Far;
    data.projection = glm::perspectiveRH_ZO(glm::radians(m_Fov), SafeAspect(aspectRatio), m_Near, m_Far);
    data.projection[1][1] *= -1.0f; // Vulkan clip-space Y flip (matches BuildCameraRenderData)
    return data;
}

glm::mat4 EditorCamera::GizmoProjection(float aspectRatio) const {
    // No Y flip: ImGuizmo and the picking unprojection use the standard GL-style
    // convention, which lines up with the flipped image the renderer presents.
    return glm::perspectiveRH_ZO(glm::radians(m_Fov), SafeAspect(aspectRatio), m_Near, m_Far);
}

void EditorCamera::Ray(const glm::vec2& viewportUV, float aspectRatio, glm::vec3& outOrigin,
                       glm::vec3& outDirection) const {
    const float ndcX = viewportUV.x * 2.0f - 1.0f;
    const float ndcY = 1.0f - viewportUV.y * 2.0f; // flip V to an upward NDC y
    const glm::mat4 invViewProj = glm::inverse(GizmoProjection(aspectRatio) * ViewMatrix());

    glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f); // ZO near plane
    glm::vec4 farPoint = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    outOrigin = glm::vec3(nearPoint);
    outDirection = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
}

} // namespace Hockey
