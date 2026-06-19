#include "Hockey/Gameplay/Rink/OutOfPlaySystem.hpp"

#include <cmath>
#include <limits>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/ResetSystem.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

bool MatchCanHandleOutOfPlay(Scene& scene) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return false;
    }

    const MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    return state.phase == MatchPhase::Playing;
}

bool OutsidePlayArea(Scene& scene, const glm::vec3& puckPosition) {
    auto view = scene.Registry().view<PlayAreaComponent, TransformComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return false;
    }

    const PlayAreaComponent& playArea = view.get<PlayAreaComponent>(*it);
    const glm::vec3 center = view.get<TransformComponent>(*it).localPosition;
    const glm::vec3 delta = puckPosition - center;
    return std::fabs(delta.x) > playArea.halfExtents.x || std::fabs(delta.y) > playArea.halfExtents.y ||
           std::fabs(delta.z) > playArea.halfExtents.z;
}

float OutOfPlayY(Scene& scene, const GameplaySettings& settings) {
    auto view = scene.Registry().view<OutOfPlayComponent>();
    const auto it = view.begin();
    if (it != view.end()) {
        return view.get<OutOfPlayComponent>(*it).minY;
    }
    return settings.allowOutOfPlay ? -5.0f : -std::numeric_limits<float>::max();
}

} // namespace

bool OutOfPlaySystem::IsPuckOutOfPlay(Scene& scene, Entity puck, const GameplaySettings& settings) {
    if (!settings.allowOutOfPlay || !puck.IsValid() || !puck.HasComponent<TransformComponent>()) {
        return false;
    }

    const glm::vec3 puckPosition = puck.GetComponent<TransformComponent>().localPosition;
    return puckPosition.y < OutOfPlayY(scene, settings) || OutsidePlayArea(scene, puckPosition);
}

void OutOfPlaySystem::HandleOutOfPlay(Scene& scene, const GameplaySettings& settings, GameplayEventQueue& events) {
    if (!MatchCanHandleOutOfPlay(scene)) {
        return;
    }

    Entity puck = FindPuck(scene);
    if (!IsPuckOutOfPlay(scene, puck, settings)) {
        return;
    }

    if (puck.HasComponent<PuckGameplayComponent>()) {
        PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
        gameplay.inPlay = false;
        gameplay.state = PuckState::Resetting;
    }
    events.Push({GameplayEventType::PuckOutOfPlay, puck.GetUUID(), UUID(0), GameplayTeam::None,
                 puck.GetComponent<TransformComponent>().localPosition});
    ResetSystem::BeginReset(scene, events);
}

} // namespace Hockey
