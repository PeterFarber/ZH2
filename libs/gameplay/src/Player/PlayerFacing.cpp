#include "Hockey/Gameplay/Player/PlayerFacing.hpp"

#include <cmath>

#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

glm::vec3 NormalizedDirectionToward(const glm::vec3& from, const glm::vec3& target) {
    glm::vec3 direction = target - from;
    direction.y = 0.0f;
    return glm::length(direction) > 0.0001f ? glm::normalize(direction) : glm::vec3{0.0f, 0.0f, 1.0f};
}

float YawDegreesFromFacing(const glm::vec3& facingDirection) {
    constexpr float kRadiansToDegrees = 57.29577951308232f;
    const glm::vec3 facing = glm::length(facingDirection) > 0.0001f ? glm::normalize(facingDirection)
                                                                    : glm::vec3{0.0f, 0.0f, 1.0f};
    return std::atan2(facing.x, facing.z) * kRadiansToDegrees;
}

} // namespace

void FacePlayerToward(Entity player, const glm::vec3& targetPosition) {
    if (!player.IsValid() || !player.HasComponent<PlayerRuntimeComponent>() ||
        !player.HasComponent<TransformComponent>()) {
        return;
    }

    TransformComponent& transform = player.GetComponent<TransformComponent>();
    PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
    runtime.facingDirection = NormalizedDirectionToward(transform.localPosition, targetPosition);
    transform.localRotation = {0.0f, YawDegreesFromFacing(runtime.facingDirection), 0.0f};
}

} // namespace Hockey
