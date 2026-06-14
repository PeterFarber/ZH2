#include "Hockey/Renderer/Camera.hpp"

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/Transform.hpp"

namespace Hockey {

CameraRenderData BuildCameraRenderData(const TransformComponent& transform, const CameraComponent& camera,
                                       float aspectRatio) {
    CameraRenderData data;

    // The camera's world transform places it in the scene; the view matrix is
    // its inverse. (Hierarchy-aware world transforms are resolved by the caller
    // via Scene::GetWorldTransform when the camera is parented.)
    const glm::mat4 world = transform.LocalMatrix();
    data.view = glm::inverse(world);
    data.position = glm::vec3(world[3]);

    data.nearClip = std::max(camera.nearClip, 1e-3f);
    data.farClip = std::max(camera.farClip, data.nearClip + 1e-2f);

    const float aspect = aspectRatio > 0.0f ? aspectRatio : 1.0f;
    // Right-handed projection with a [0,1] depth range (Vulkan convention), then
    // flip Y so +Y is up in clip space matching the framebuffer orientation.
    data.projection = glm::perspectiveRH_ZO(glm::radians(camera.fovDegrees), aspect, data.nearClip, data.farClip);
    data.projection[1][1] *= -1.0f;

    return data;
}

bool FindActiveCamera(const Scene& scene, float aspectRatio, CameraRenderData& out) {
    const auto view = scene.Registry().view<const TransformComponent, const CameraComponent>();
    entt::entity chosen = entt::null;
    for (const entt::entity handle : view) {
        const Entity entity{handle, const_cast<Scene*>(&scene)};
        if (!entity.IsActiveInHierarchy()) {
            continue;
        }
        if (chosen == entt::null) {
            chosen = handle; // first active camera as the fallback
        }
        if (view.get<const CameraComponent>(handle).primary) {
            chosen = handle; // an explicit primary camera wins
            break;
        }
    }
    if (chosen == entt::null) {
        return false;
    }

    const Entity entity{chosen, const_cast<Scene*>(&scene)};
    const CameraComponent& camera = view.get<const CameraComponent>(chosen);

    // Build a transform whose local matrix equals the camera's world matrix so
    // parented cameras resolve correctly.
    TransformComponent worldTransform;
    DecomposeTransform(scene.GetWorldTransform(entity), worldTransform.localPosition, worldTransform.localRotation,
                       worldTransform.localScale);
    out = BuildCameraRenderData(worldTransform, camera, aspectRatio);
    return true;
}

} // namespace Hockey
