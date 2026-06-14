#pragma once

#include <glm/glm.hpp>

namespace Hockey {

struct TransformComponent;
struct CameraComponent;
class Scene;

// View/projection data consumed by the renderer for a single camera.
struct CameraRenderData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec3 position{0.0f};
    float nearClip = 0.1f;
    float farClip = 1000.0f;
};

// Builds camera render data from ECS components. Uses a right-handed,
// reverse-friendly perspective with Vulkan clip-space Y handled by the caller.
CameraRenderData BuildCameraRenderData(const TransformComponent& transform, const CameraComponent& camera,
                                       float aspectRatio);

// Resolves the scene's active camera (the first active CameraComponent flagged
// primary, otherwise the first active one), accounting for hierarchy. Returns
// false when the scene has no usable camera, leaving 'out' untouched.
bool FindActiveCamera(const Scene& scene, float aspectRatio, CameraRenderData& out);

} // namespace Hockey
