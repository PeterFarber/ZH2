#include "Hockey/Gameplay/Match/ScoreSystem.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/ResetSystem.hpp"

namespace Hockey {
namespace {

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

void UpdateTeamScore(Scene& scene, GameplayTeam team, uint32_t score) {
    auto view = scene.Registry().view<TeamStateComponent>();
    for (const entt::entity handle : view) {
        TeamStateComponent& teamState = view.get<TeamStateComponent>(handle);
        if (teamState.team == team) {
            teamState.score = score;
            return;
        }
    }
}

} // namespace

void ScoreSystem::AddGoal(Scene& scene,
                          GameplayTeam scoringTeam,
                          const GameplaySettings& settings,
                          GameplayEventQueue& events) {
    Entity match = FindMatch(scene);
    if (!match.IsValid() || scoringTeam == GameplayTeam::None) {
        return;
    }

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    if (state.phase == MatchPhase::GoalScored || state.phase == MatchPhase::MatchEnded) {
        return;
    }

    if (scoringTeam == GameplayTeam::Home) {
        ++state.homeScore;
    } else if (scoringTeam == GameplayTeam::Away) {
        ++state.awayScore;
    }
    state.clockRunning = !settings.stopClockAfterGoal;

    ScoreComponent& score = match.HasComponent<ScoreComponent>() ? match.GetComponent<ScoreComponent>()
                                                                 : match.AddComponent<ScoreComponent>();
    score.homeScore = state.homeScore;
    score.awayScore = state.awayScore;
    UpdateTeamScore(scene, GameplayTeam::Home, state.homeScore);
    UpdateTeamScore(scene, GameplayTeam::Away, state.awayScore);

    events.Push({GameplayEventType::GoalScored, match.GetUUID(), UUID(0), scoringTeam});
    events.Push({GameplayEventType::ScoreChanged, match.GetUUID(), UUID(0), scoringTeam});

    if (settings.autoFaceoffAfterGoal) {
        ResetSystem::BeginReset(scene, events);
    } else {
        state.phase = MatchPhase::GoalScored;
        state.phaseTimer = 0.0f;
    }
}

uint32_t ScoreSystem::GetScore(Scene& scene, GameplayTeam team) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return 0;
    }

    const MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    if (team == GameplayTeam::Home) {
        return state.homeScore;
    }
    if (team == GameplayTeam::Away) {
        return state.awayScore;
    }
    return 0;
}

} // namespace Hockey
