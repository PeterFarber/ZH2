#include "Hockey/Gameplay/Stick/PassingSystem.hpp"

#include <cmath>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

glm::vec3 DirectionFromInputOrFacing(Entity player, const GameplayInputFrame& input) {
    if (glm::dot(input.aim, input.aim) > 0.0001f) {
        return glm::normalize(glm::vec3{input.aim.x, 0.0f, input.aim.y});
    }
    if (player.HasComponent<PlayerRuntimeComponent>()) {
        const glm::vec3 facing = player.GetComponent<PlayerRuntimeComponent>().facingDirection;
        if (glm::length(facing) > 0.0001f) {
            return glm::normalize(facing);
        }
    }
    return {0.0f, 0.0f, 1.0f};
}

Entity FindPassTarget(Scene& scene, Entity passer, const glm::vec3& direction, const GameplayTuning& tuning) {
    const PlayerComponent& passerComponent = passer.GetComponent<PlayerComponent>();
    const glm::vec3 passerPosition = passer.GetComponent<TransformComponent>().localPosition;
    const float minDot = std::cos(glm::radians(tuning.pass.maxAssistAngleDegrees));

    Entity best;
    float bestDistance = 0.0f;
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        Entity candidate(handle, &scene);
        if (candidate == passer) {
            continue;
        }
        const PlayerComponent& player = candidate.GetComponent<PlayerComponent>();
        if (player.team != passerComponent.team || !player.activeInMatch) {
            continue;
        }

        const glm::vec3 offset = candidate.GetComponent<TransformComponent>().localPosition - passerPosition;
        const float distance = glm::length(offset);
        if (distance <= 0.0001f) {
            continue;
        }

        const float dot = glm::dot(glm::normalize(offset), direction);
        if (dot < minDot) {
            continue;
        }
        if (!best.IsValid() || distance < bestDistance) {
            best = candidate;
            bestDistance = distance;
        }
    }
    return best;
}

} // namespace

void PassingSystem::FixedUpdate(Scene& scene,
                                const GameplayInputBuffer& inputs,
                                const GameplayTuning& tuning,
                                GameplayEventQueue& events) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>() || !puck.HasComponent<PuckRuntimeComponent>()) {
        return;
    }

    PuckGameplayComponent& puckGameplay = puck.GetComponent<PuckGameplayComponent>();
    if (puckGameplay.state != PuckState::Possessed || !puckGameplay.possessingPlayer.IsValid()) {
        return;
    }

    Entity passer = scene.FindEntityByUUID(puckGameplay.possessingPlayer);
    if (!passer.IsValid() || !passer.HasComponent<PlayerComponent>() || !passer.HasComponent<PassComponent>() ||
        !passer.HasComponent<StickComponent>() || !passer.GetComponent<StickComponent>().canPass) {
        return;
    }

    GameplayInputFrame input;
    if (!inputs.GetInput(passer.GetComponent<PlayerComponent>().playerIndex, input) ||
        (!input.passPressed && !input.passReleased)) {
        return;
    }

    const glm::vec3 direction = DirectionFromInputOrFacing(passer, input);
    Entity target = FindPassTarget(scene, passer, direction, tuning);
    passer.GetComponent<PassComponent>().targetPlayer = target.IsValid() ? target.GetUUID() : UUID(0);

    PuckPossession::Release(scene, puck, events);
    puckGameplay.state = PuckState::Passed;
    puckGameplay.lastTouchedPlayer = passer.GetUUID();
    puckGameplay.lastTouchedTeam = passer.GetComponent<PlayerComponent>().team;
    puck.GetComponent<PuckRuntimeComponent>().velocity = direction * tuning.pass.power;

    events.Push({GameplayEventType::PuckPassed, puck.GetUUID(), target.IsValid() ? target.GetUUID() : passer.GetUUID(),
                 puckGameplay.lastTouchedTeam,
                 puck.HasComponent<TransformComponent>() ? puck.GetComponent<TransformComponent>().localPosition : glm::vec3{0.0f}});
}

} // namespace Hockey
