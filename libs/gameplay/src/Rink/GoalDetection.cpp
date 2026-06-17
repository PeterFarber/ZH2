#include "Hockey/Gameplay/Rink/GoalDetection.hpp"

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/ScoreSystem.hpp"

namespace Hockey {
namespace {

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

bool CanScore(Scene& scene) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return false;
    }

    const MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    return state.phase != MatchPhase::GoalScored && state.phase != MatchPhase::MatchEnded &&
           state.phase != MatchPhase::FaceoffSetup && state.phase != MatchPhase::Faceoff;
}

} // namespace

bool GoalDetection::HandleGoalTrigger(Scene& scene,
                                      Entity goal,
                                      Entity other,
                                      const GameplaySettings& settings,
                                      GameplayEventQueue& events) {
    if (!CanScore(scene) || !goal.IsValid() || !other.IsValid() || !goal.HasComponent<GoalGameplayComponent>() ||
        !other.HasComponent<PuckComponent>()) {
        return false;
    }

    const GoalGameplayComponent& goalGameplay = goal.GetComponent<GoalGameplayComponent>();
    if (!goalGameplay.enabled || goalGameplay.scoringTeam == GameplayTeam::None) {
        return false;
    }

    ScoreSystem::AddGoal(scene, goalGameplay.scoringTeam, settings, events);
    return true;
}

void GoalDetection::FixedUpdate(Scene& scene, const GameplaySettings& settings, GameplayEventQueue& events) {
    if (!CanScore(scene)) {
        return;
    }

    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<TransformComponent>()) {
        return;
    }

    const glm::vec3 puckPosition = puck.GetComponent<TransformComponent>().localPosition;
    auto goals = scene.Registry().view<GoalGameplayComponent, TransformComponent>();
    for (const entt::entity handle : goals) {
        Entity goal(handle, &scene);
        const GoalGameplayComponent& gameplay = goal.GetComponent<GoalGameplayComponent>();
        if (!gameplay.enabled) {
            continue;
        }

        const glm::vec3 goalPosition = goal.GetComponent<TransformComponent>().localPosition;
        if (glm::length(puckPosition - goalPosition) <= 1.0f) {
            ScoreSystem::AddGoal(scene, gameplay.scoringTeam, settings, events);
            return;
        }
    }
}

} // namespace Hockey
