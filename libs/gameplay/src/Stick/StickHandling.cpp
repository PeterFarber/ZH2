#include "Hockey/Gameplay/Stick/StickHandling.hpp"

#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

glm::vec3 StickHandling::GetStickWorldPosition(Entity player) {
    if (!player.IsValid() || !player.HasComponent<TransformComponent>() || !player.HasComponent<StickComponent>()) {
        return glm::vec3{0.0f};
    }

    const StickComponent& stick = player.GetComponent<StickComponent>();
    StickTuning tuning;
    tuning.reach = stick.reach;
    tuning.width = stick.width;
    tuning.localOffset = stick.localOffset;
    return GetStickWorldPosition(player, tuning);
}

glm::vec3 StickHandling::GetStickWorldPosition(Entity player, const StickTuning& tuning) {
    if (!player.IsValid() || !player.HasComponent<TransformComponent>() || !player.HasComponent<StickComponent>()) {
        return glm::vec3{0.0f};
    }

    const TransformComponent& transform = player.GetComponent<TransformComponent>();

    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    if (player.HasComponent<PlayerRuntimeComponent>()) {
        const glm::vec3 facing = player.GetComponent<PlayerRuntimeComponent>().facingDirection;
        if (glm::length(facing) > 0.0001f) {
            forward = glm::normalize(facing);
        }
    }

    const glm::vec3 right{forward.z, 0.0f, -forward.x};
    return transform.localPosition + right * tuning.localOffset.x + glm::vec3{0.0f, tuning.localOffset.y, 0.0f} +
           forward * tuning.localOffset.z;
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

    StickTuning tuning;
    tuning.reach = stick.reach;
    tuning.width = stick.width;
    tuning.localOffset = stick.localOffset;
    return CanControlPuck(player, puck, tuning);
}

bool StickHandling::CanControlPuck(Entity player, Entity puck, const StickTuning& tuning) {
    if (!player.IsValid() || !puck.IsValid() || !player.HasComponent<StickComponent>() ||
        !puck.HasComponent<TransformComponent>()) {
        return false;
    }

    const StickComponent& stick = player.GetComponent<StickComponent>();
    if (!stick.canControlPuck) {
        return false;
    }

    const glm::vec3 stickPosition = GetStickWorldPosition(player, tuning);
    const glm::vec3 puckPosition = puck.GetComponent<TransformComponent>().localPosition;
    const float distance = glm::length(puckPosition - stickPosition);
    return distance <= tuning.reach;
}

} // namespace Hockey
