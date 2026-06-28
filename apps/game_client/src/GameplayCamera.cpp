#include "GameplayCamera.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/Transform.hpp"
#include "Hockey/Gameplay/Player/PlayerComponents.hpp"

#include <algorithm>
#include <cmath>

#include <glm/mat4x4.hpp>

namespace HockeyClient {
namespace {

Hockey::Entity FindFollowTarget(Hockey::Scene& scene, uint32_t playerIndex) {
    auto view = scene.Registry().view<Hockey::PlayerComponent, Hockey::TransformComponent>();
    for (const entt::entity handle : view) {
        Hockey::Entity player(handle, &scene);
        if (!player.IsActiveInHierarchy()) {
            continue;
        }
        if (view.get<Hockey::PlayerComponent>(handle).playerIndex == playerIndex) {
            return player;
        }
    }
    return {};
}

Hockey::Entity FindActiveCameraEntity(Hockey::Scene& scene) {
    auto view = scene.Registry().view<Hockey::TransformComponent, Hockey::CameraComponent>();
    entt::entity chosen = entt::null;
    for (const entt::entity handle : view) {
        Hockey::Entity camera(handle, &scene);
        if (!camera.IsActiveInHierarchy()) {
            continue;
        }
        if (chosen == entt::null) {
            chosen = handle;
        }
        if (view.get<Hockey::CameraComponent>(handle).primary) {
            chosen = handle;
            break;
        }
    }
    return chosen == entt::null ? Hockey::Entity{} : Hockey::Entity(chosen, &scene);
}

float ResponseAlpha(float response, float deltaSeconds) {
    const float clampedDelta = std::max(deltaSeconds, 0.0f);
    const float clampedResponse = std::max(response, 0.0f);
    return 1.0f - std::exp(-clampedResponse * clampedDelta);
}

glm::vec3 SmoothVector(const glm::vec3& current, const glm::vec3& desired, float response, float deltaSeconds) {
    const float alpha = ResponseAlpha(response, deltaSeconds);
    return current + (desired - current) * alpha;
}

} // namespace

bool UpdateGameplayFollowCamera(Hockey::Scene& scene,
                                float deltaSeconds,
                                const GameplayFollowCameraSettings& settings,
                                GameplayFollowCameraState& state) {
    if (!settings.enabled) {
        return false;
    }

    const Hockey::Entity target = FindFollowTarget(scene, settings.playerIndex);
    const Hockey::Entity camera = FindActiveCameraEntity(scene);
    if (!target || !camera) {
        return false;
    }
    const Hockey::CameraComponent& cameraComponent = camera.GetComponent<Hockey::CameraComponent>();
    if (!cameraComponent.followPlayer) {
        return false;
    }

    const glm::vec3 targetPosition = glm::vec3(scene.GetWorldTransform(target)[3]);
    const glm::vec3 desiredPosition = targetPosition + cameraComponent.followOffset;

    if (!state.initialized) {
        state.position = desiredPosition;
        state.initialized = true;
    } else {
        state.position = SmoothVector(state.position, desiredPosition, settings.positionResponse, deltaSeconds);
    }

    const glm::vec3 scale = camera.GetComponent<Hockey::TransformComponent>().localScale;
    scene.SetWorldTransform(camera, Hockey::ComposeTransform(state.position, cameraComponent.followRotation, scale));
    return true;
}

} // namespace HockeyClient
