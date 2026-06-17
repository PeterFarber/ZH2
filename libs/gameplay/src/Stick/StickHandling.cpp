#include "Hockey/Gameplay/Stick/StickHandling.hpp"

#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {

glm::vec3 StickHandling::GetStickWorldPosition(Entity player) {
    if (!player.IsValid() || !player.HasComponent<TransformComponent>() || !player.HasComponent<StickComponent>()) {
        return glm::vec3{0.0f};
    }

    const TransformComponent& transform = player.GetComponent<TransformComponent>();
    const StickComponent& stick = player.GetComponent<StickComponent>();

    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    if (player.HasComponent<PlayerRuntimeComponent>()) {
        const glm::vec3 facing = player.GetComponent<PlayerRuntimeComponent>().facingDirection;
        if (glm::length(facing) > 0.0001f) {
            forward = glm::normalize(facing);
        }
    }

    const glm::vec3 right{forward.z, 0.0f, -forward.x};
    return transform.localPosition + right * stick.localOffset.x + glm::vec3{0.0f, stick.localOffset.y, 0.0f} +
           forward * stick.localOffset.z;
}

bool StickHandling::CanControlPuck(Entity player, Entity puck) {
    if (!player.IsValid() || !puck.IsValid() || !player.HasComponent<StickComponent>() ||
        !puck.HasComponent<TransformComponent>()) {
        return false;
    }

    const StickComponent& stick = player.GetComponent<StickComponent>();
    if (!stick.canControlPuck) {
        return false;
    }

    const glm::vec3 stickPosition = GetStickWorldPosition(player);
    const glm::vec3 puckPosition = puck.GetComponent<TransformComponent>().localPosition;
    const float distance = glm::length(puckPosition - stickPosition);
    return distance <= stick.reach;
}

} // namespace Hockey
