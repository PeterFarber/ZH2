#include "Hockey/Gameplay/Stick/CheckingSystem.hpp"

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

glm::vec3 FacingOrForward(Entity player) {
    if (player.HasComponent<PlayerRuntimeComponent>()) {
        const glm::vec3 facing = player.GetComponent<PlayerRuntimeComponent>().facingDirection;
        if (glm::length(facing) > 0.0001f) {
            return glm::normalize(facing);
        }
    }
    return {0.0f, 0.0f, 1.0f};
}

Entity FindCheckTarget(Scene& scene, Entity checker, const GameplayTuning& tuning) {
    const PlayerComponent& checkerPlayer = checker.GetComponent<PlayerComponent>();
    const glm::vec3 checkerPosition = checker.GetComponent<TransformComponent>().localPosition;
    const glm::vec3 facing = FacingOrForward(checker);

    Entity best;
    float bestDistance = 0.0f;
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        Entity target(handle, &scene);
        if (target == checker) {
            continue;
        }

        const PlayerComponent& targetPlayer = target.GetComponent<PlayerComponent>();
        if (targetPlayer.team == checkerPlayer.team || !targetPlayer.activeInMatch) {
            continue;
        }

        const glm::vec3 offset = target.GetComponent<TransformComponent>().localPosition - checkerPosition;
        const float distance = glm::length(offset);
        if (distance > tuning.check.radius || distance <= 0.0001f) {
            continue;
        }

        if (glm::dot(glm::normalize(offset), facing) < 0.0f) {
            continue;
        }

        if (!best.IsValid() || distance < bestDistance) {
            best = target;
            bestDistance = distance;
        }
    }
    return best;
}

bool PlayerPossessesPuck(Entity player, Entity puck) {
    return player.IsValid() && puck.IsValid() && puck.HasComponent<PuckGameplayComponent>() &&
           puck.GetComponent<PuckGameplayComponent>().possessingPlayer == player.GetUUID();
}

void TickCooldowns(PlayerRuntimeComponent& runtime, CheckComponent& check, float fixedDeltaSeconds) {
    runtime.checkCooldown = glm::max(0.0f, runtime.checkCooldown - fixedDeltaSeconds);
    runtime.pokeCheckCooldown = glm::max(0.0f, runtime.pokeCheckCooldown - fixedDeltaSeconds);
    check.cooldown = glm::max(0.0f, check.cooldown - fixedDeltaSeconds);
}

} // namespace

void CheckingSystem::FixedUpdate(Scene& scene,
                                 const GameplayInputBuffer& inputs,
                                 const GameplaySettings& settings,
                                 const GameplayTuning& tuning,
                                 float fixedDeltaSeconds,
                                 GameplayEventQueue& events) {
    Entity puck = FindPuck(scene);
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, CheckComponent, StickComponent, TransformComponent>();

    for (const entt::entity handle : view) {
        Entity checker(handle, &scene);
        PlayerRuntimeComponent& runtime = checker.GetComponent<PlayerRuntimeComponent>();
        CheckComponent& check = checker.GetComponent<CheckComponent>();
        TickCooldowns(runtime, check, fixedDeltaSeconds);

        GameplayInputFrame input;
        if (!inputs.GetInput(checker.GetComponent<PlayerComponent>().playerIndex, input) || !runtime.inputEnabled) {
            continue;
        }

        const StickComponent& stick = checker.GetComponent<StickComponent>();
        const bool wantsSteal = input.stealPressed || input.stealHeld;
        if (wantsSteal && stick.canCheck && puck.IsValid() && puck.HasComponent<PuckGameplayComponent>() &&
            runtime.pokeCheckCooldown <= 0.0f && !PlayerPossessesPuck(checker, puck)) {
            const bool canReachPuck = StickHandling::CanControlPuck(checker, puck);
            if (canReachPuck) {
                PuckPossession::Release(scene, puck, events);
            }
            runtime.pokeCheckCooldown = tuning.skater.stealCooldownSeconds;
            events.Push({GameplayEventType::StealAttempted,
                         checker.GetUUID(),
                         puck.GetUUID(),
                         checker.GetComponent<PlayerComponent>().team,
                         checker.GetComponent<TransformComponent>().localPosition});
        }

        if (input.pokeCheckPressed && stick.canCheck && puck.IsValid() && puck.HasComponent<PuckGameplayComponent>() &&
            runtime.pokeCheckCooldown <= 0.0f && StickHandling::CanControlPuck(checker, puck)) {
            PuckPossession::Release(scene, puck, events);
            runtime.pokeCheckCooldown = tuning.stick.pokeCheckCooldown;
        }

        if (!input.checkPressed || !settings.allowBodyChecking || !stick.canCheck || runtime.checkCooldown > 0.0f ||
            check.cooldown > 0.0f) {
            continue;
        }

        Entity target = FindCheckTarget(scene, checker, tuning);
        if (!target.IsValid()) {
            continue;
        }

        if (target.HasComponent<PlayerRuntimeComponent>()) {
            target.GetComponent<PlayerRuntimeComponent>().velocity += FacingOrForward(checker) * tuning.check.impulse;
        }

        runtime.checkCooldown = tuning.check.cooldown;
        check.cooldown = tuning.check.cooldown;
        events.Push({GameplayEventType::PlayerChecked, checker.GetUUID(), target.GetUUID(),
                     checker.GetComponent<PlayerComponent>().team, target.GetComponent<TransformComponent>().localPosition});
    }
}

} // namespace Hockey
