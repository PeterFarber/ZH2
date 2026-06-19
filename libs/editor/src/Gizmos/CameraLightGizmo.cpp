#include "Hockey/Editor/Gizmos/CameraLightGizmo.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/gtc/constants.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Selection.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"

namespace Hockey::CameraLightGizmo {

namespace {

constexpr glm::vec4 kDetailColor{1.0f, 1.0f, 1.0f, 1.0f};
constexpr int kRingSegments = 32;

glm::vec3 SafeAxis(const glm::mat4& matrix, int column, glm::vec3 fallback) {
    const glm::vec3 axis(matrix[column]);
    const float len = glm::length(axis);
    return len > 1e-5f ? axis / len : fallback;
}

struct Basis {
    glm::vec3 position{0.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
};

Basis BasisFromWorld(const glm::mat4& world) {
    Basis basis;
    basis.position = glm::vec3(world[3]);
    basis.right = SafeAxis(world, 0, basis.right);
    basis.up = SafeAxis(world, 1, basis.up);
    const glm::vec3 z = SafeAxis(world, 2, glm::vec3(0.0f, 0.0f, 1.0f));
    basis.forward = -z;
    return basis;
}

void DrawRing(DebugDraw& debug, glm::vec3 center, glm::vec3 axisU, glm::vec3 axisV, float radius, glm::vec4 color,
              int segments = kRingSegments) {
    radius = std::max(radius, 0.0f);
    segments = std::max(segments, 3);

    glm::vec3 prev = center + axisU * radius;
    for (int i = 1; i <= segments; ++i) {
        const float t = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
        const glm::vec3 next = center + (axisU * std::cos(t) + axisV * std::sin(t)) * radius;
        debug.DrawLine(prev, next, color);
        prev = next;
    }
}

void DrawCameraFrustum(DebugDraw& debug, const Basis& basis, const CameraComponent& camera, float viewportAspect) {
    const float aspect = viewportAspect > 0.0f ? viewportAspect : 1.0f;
    const float nearClip = std::max(camera.nearClip, 1e-3f);
    const float farClip = std::max(camera.farClip, nearClip + 1e-2f);
    const float halfFov = glm::radians(std::clamp(camera.fovDegrees, 1.0f, 179.0f)) * 0.5f;

    const float nearHalfHeight = std::tan(halfFov) * nearClip;
    const float nearHalfWidth = nearHalfHeight * aspect;
    const float farHalfHeight = std::tan(halfFov) * farClip;
    const float farHalfWidth = farHalfHeight * aspect;

    const glm::vec3 nearCenter = basis.position + basis.forward * nearClip;
    const glm::vec3 farCenter = basis.position + basis.forward * farClip;

    const glm::vec3 ntl = nearCenter + basis.up * nearHalfHeight - basis.right * nearHalfWidth;
    const glm::vec3 ntr = nearCenter + basis.up * nearHalfHeight + basis.right * nearHalfWidth;
    const glm::vec3 nbl = nearCenter - basis.up * nearHalfHeight - basis.right * nearHalfWidth;
    const glm::vec3 nbr = nearCenter - basis.up * nearHalfHeight + basis.right * nearHalfWidth;

    const glm::vec3 ftl = farCenter + basis.up * farHalfHeight - basis.right * farHalfWidth;
    const glm::vec3 ftr = farCenter + basis.up * farHalfHeight + basis.right * farHalfWidth;
    const glm::vec3 fbl = farCenter - basis.up * farHalfHeight - basis.right * farHalfWidth;
    const glm::vec3 fbr = farCenter - basis.up * farHalfHeight + basis.right * farHalfWidth;

    debug.DrawLine(ntl, ntr, kDetailColor);
    debug.DrawLine(ntr, nbr, kDetailColor);
    debug.DrawLine(nbr, nbl, kDetailColor);
    debug.DrawLine(nbl, ntl, kDetailColor);

    debug.DrawLine(ftl, ftr, kDetailColor);
    debug.DrawLine(ftr, fbr, kDetailColor);
    debug.DrawLine(fbr, fbl, kDetailColor);
    debug.DrawLine(fbl, ftl, kDetailColor);

    debug.DrawLine(ntl, ftl, kDetailColor);
    debug.DrawLine(ntr, ftr, kDetailColor);
    debug.DrawLine(nbl, fbl, kDetailColor);
    debug.DrawLine(nbr, fbr, kDetailColor);
}

void DrawSpotLight(DebugDraw& debug, const Basis& basis, const LightComponent& light) {
    const float range = std::max(light.range, 0.0f);
    const float outerAngle = glm::radians(std::clamp(light.outerConeDegrees, 0.1f, 89.0f));
    const float innerAngle = glm::radians(std::clamp(light.innerConeDegrees, 0.1f, 89.0f));
    const float outerRadius = std::tan(outerAngle) * range;
    const float innerRadius = std::tan(std::min(innerAngle, outerAngle)) * range;
    const glm::vec3 capCenter = basis.position + basis.forward * range;

    DrawRing(debug, capCenter, basis.right, basis.up, outerRadius, kDetailColor);
    if (innerRadius > 1e-4f && innerRadius < outerRadius - 1e-4f) {
        DrawRing(debug, capCenter, basis.right, basis.up, innerRadius, kDetailColor);
    }

    debug.DrawLine(basis.position, capCenter, kDetailColor);
    debug.DrawLine(basis.position, capCenter + basis.right * outerRadius, kDetailColor);
    debug.DrawLine(basis.position, capCenter - basis.right * outerRadius, kDetailColor);
    debug.DrawLine(basis.position, capCenter + basis.up * outerRadius, kDetailColor);
    debug.DrawLine(basis.position, capCenter - basis.up * outerRadius, kDetailColor);
}

void DrawDirectionalLight(DebugDraw& debug, const Basis& basis) {
    constexpr float kLength = 3.0f;
    constexpr float kSpread = 0.4f;
    const glm::vec3 offsets[] = {
        glm::vec3(0.0f),
        basis.right * kSpread,
        -basis.right * kSpread,
        basis.up * kSpread,
        -basis.up * kSpread,
    };
    for (const glm::vec3& offset : offsets) {
        const glm::vec3 start = basis.position + offset;
        debug.DrawLine(start, start + basis.forward * kLength, kDetailColor);
    }
}

bool ProjectToPixels(const CameraRenderData& camera, glm::vec3 worldPosition, glm::vec2 viewportPixels,
                     glm::vec2& outPixels, float& outDepth) {
    if (viewportPixels.x <= 0.0f || viewportPixels.y <= 0.0f) {
        return false;
    }

    const glm::vec4 clip = camera.projection * camera.view * glm::vec4(worldPosition, 1.0f);
    if (clip.w <= 1e-5f) {
        return false;
    }
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.z < 0.0f || ndc.z > 1.0f) {
        return false;
    }

    outPixels = glm::vec2((ndc.x * 0.5f + 0.5f) * viewportPixels.x, (ndc.y * 0.5f + 0.5f) * viewportPixels.y);
    outDepth = ndc.z;
    return true;
}

bool IsCameraOrLight(Entity entity) {
    return entity.HasComponent<CameraComponent>() || entity.HasComponent<LightComponent>();
}

} // namespace

void Submit(DebugDraw& debug, Scene& scene, const Selection& selection, float viewportAspect) {
    for (Entity entity : scene.GetAllEntities()) {
        if (!entity.HasComponent<TransformComponent>() || !IsCameraOrLight(entity)) {
            continue;
        }

        const Basis basis = BasisFromWorld(scene.GetWorldTransform(entity));
        const bool selected = selection.IsSelected(entity.GetUUID());

        if (entity.HasComponent<CameraComponent>() && selected) {
            DrawCameraFrustum(debug, basis, entity.GetComponent<CameraComponent>(), viewportAspect);
        }

        if (entity.HasComponent<LightComponent>() && selected) {
            const LightComponent& light = entity.GetComponent<LightComponent>();
            switch (light.type) {
            case LightComponent::Type::Point:
                debug.DrawSphere(basis.position, std::max(light.range, 0.0f), kDetailColor);
                break;
            case LightComponent::Type::Spot:
                DrawSpotLight(debug, basis, light);
                break;
            case LightComponent::Type::Directional:
                DrawDirectionalLight(debug, basis);
                break;
            }
        }
    }
}

UUID PickOrigin(Scene& scene, const CameraRenderData& camera, glm::vec2 viewportUV, glm::vec2 viewportPixels,
                float radiusPixels) {
    if (viewportPixels.x <= 0.0f || viewportPixels.y <= 0.0f || radiusPixels <= 0.0f) {
        return UUID{0};
    }

    const glm::vec2 clickPixels(viewportUV.x * viewportPixels.x, viewportUV.y * viewportPixels.y);
    UUID best{0};
    float bestDistanceSq = radiusPixels * radiusPixels;
    float bestDepth = std::numeric_limits<float>::max();

    for (Entity entity : scene.GetAllEntities()) {
        if (!entity.HasComponent<TransformComponent>() || !IsCameraOrLight(entity)) {
            continue;
        }

        const glm::vec3 position(scene.GetWorldTransform(entity)[3]);
        glm::vec2 pixels;
        float depth = 0.0f;
        if (!ProjectToPixels(camera, position, viewportPixels, pixels, depth)) {
            continue;
        }

        const glm::vec2 delta = pixels - clickPixels;
        const float distanceSq = glm::dot(delta, delta);
        if (distanceSq < bestDistanceSq || (std::abs(distanceSq - bestDistanceSq) <= 1e-4f && depth < bestDepth)) {
            bestDistanceSq = distanceSq;
            bestDepth = depth;
            best = entity.GetUUID();
        }
    }

    return best;
}

} // namespace Hockey::CameraLightGizmo
