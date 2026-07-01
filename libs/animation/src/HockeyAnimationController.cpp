#include "Hockey/Animation/HockeyAnimationController.hpp"

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

#include <algorithm>
#include <glm/geometric.hpp>

namespace Hockey {
namespace {

AnimationParameterComponent& EnsureParameters(Entity entity) {
    if (!entity.HasComponent<AnimationParameterComponent>()) {
        return entity.AddComponent<AnimationParameterComponent>();
    }
    return entity.GetComponent<AnimationParameterComponent>();
}

void ApplyTrigger(AnimationParameterComponent& parameters, HockeyAnimationTriggerType type) {
    switch (type) {
    case HockeyAnimationTriggerType::Shoot:
        parameters.shooting = true;
        break;
    case HockeyAnimationTriggerType::Steal:
        parameters.stealing = true;
        break;
    case HockeyAnimationTriggerType::GoalieSave:
        parameters.goalieAction = true;
        break;
    case HockeyAnimationTriggerType::Celebrate:
        parameters.celebrating = true;
        break;
    }
}

} // namespace

void HockeyAnimationController::Apply(Scene& scene, const HockeyAnimationFrame& frame) const {
    auto parameterView = scene.Registry().view<AnimationParameterComponent>();
    for (entt::entity handle : parameterView) {
        parameterView.get<AnimationParameterComponent>(handle) = {};
    }

    for (const HockeyAnimationPlayerState& state : frame.players) {
        Entity entity = scene.FindEntityByUUID(state.entity);
        if (!entity.IsValid() || !entity.HasComponent<AnimatorComponent>()) {
            continue;
        }

        AnimationParameterComponent& parameters = EnsureParameters(entity);
        parameters.speed = glm::length(state.velocity);
        parameters.shotChargeRatio = std::clamp(state.shotChargeRatio, 0.0f, 1.0f);
        parameters.hasPuck = state.hasPuck;
        parameters.shooting = state.shotCharging;
    }

    for (const HockeyAnimationTrigger& trigger : frame.triggers) {
        Entity entity = scene.FindEntityByUUID(trigger.entity);
        if (!entity.IsValid() || !entity.HasComponent<AnimatorComponent>()) {
            continue;
        }

        AnimationParameterComponent& parameters = EnsureParameters(entity);
        ApplyTrigger(parameters, trigger.type);
    }
}

} // namespace Hockey
