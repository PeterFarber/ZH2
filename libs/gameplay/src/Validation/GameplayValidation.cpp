#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

#include <array>
#include <string>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

using Severity = SceneValidationIssue::Severity;

void AddIssue(std::vector<SceneValidationIssue>& issues, Severity severity, std::string message, UUID entityId = {}) {
    issues.push_back({severity, std::move(message), entityId});
}

GameplayTeam ToGameplayTeam(Team team) {
    switch (team) {
    case Team::Home: return GameplayTeam::Home;
    case Team::Away: return GameplayTeam::Away;
    case Team::None: return GameplayTeam::None;
    }
    return GameplayTeam::None;
}

GameplayRole ToGameplayRole(PlayerRole role) {
    return role == PlayerRole::Goalie ? GameplayRole::Goalie : GameplayRole::Skater;
}

bool HasSpawn(const entt::registry& registry, GameplayTeam team, GameplayRole role, int index) {
    auto view = registry.view<const SpawnPointComponent>();
    for (const entt::entity handle : view) {
        const auto& spawn = view.get<const SpawnPointComponent>(handle);
        if (ToGameplayTeam(spawn.team) == team && ToGameplayRole(spawn.role) == role && spawn.index == index) {
            return true;
        }
    }
    return false;
}

} // namespace

void ValidateGameplayScene(const Scene& scene, std::vector<SceneValidationIssue>& issues) {
    const entt::registry& registry = scene.Registry();

    int puckCount = 0;
    int homeGoals = 0;
    int awayGoals = 0;
    bool centerFaceoff = false;
    bool hasPlayArea = false;
    bool hasRink = false;

    auto idView = registry.view<const IDComponent>();
    for (const entt::entity handle : idView) {
        const UUID id = idView.get<const IDComponent>(handle).id;

        if (registry.all_of<PuckComponent>(handle) || registry.all_of<PuckGameplayComponent>(handle)) {
            ++puckCount;
        }
        if (const auto* goal = registry.try_get<GoalComponent>(handle)) {
            if (goal->defendingTeam == Team::Home) {
                ++homeGoals;
            } else if (goal->defendingTeam == Team::Away) {
                ++awayGoals;
            } else {
                AddIssue(issues, Severity::Error, "Gameplay goal has invalid defending team", id);
            }
        }
        if (const auto* goalGameplay = registry.try_get<GoalGameplayComponent>(handle)) {
            if (goalGameplay->defendingTeam == GameplayTeam::None) {
                AddIssue(issues, Severity::Error, "GoalGameplayComponent defending team must be Home or Away", id);
            }
        }
        if (const auto* faceoff = registry.try_get<FaceoffSpotComponent>(handle)) {
            centerFaceoff = centerFaceoff || faceoff->index == 0;
        }
        if (const auto* faceoffGameplay = registry.try_get<FaceoffGameplayComponent>(handle)) {
            centerFaceoff = centerFaceoff || faceoffGameplay->centerIce;
        }
        hasPlayArea = hasPlayArea || registry.all_of<PlayAreaComponent>(handle);
        hasRink = hasRink || registry.all_of<RinkComponent>(handle);

        if (const auto* player = registry.try_get<PlayerComponent>(handle)) {
            if (player->team == GameplayTeam::None) {
                AddIssue(issues, Severity::Error, "PlayerComponent has no team", id);
            }
        }
    }

    if (puckCount == 0) {
        AddIssue(issues, Severity::Error, "Gameplay scene is missing a puck");
    } else if (puckCount > 1) {
        AddIssue(issues, Severity::Error, "Gameplay scene has multiple pucks");
    }
    if (homeGoals != 1) {
        AddIssue(issues, Severity::Error, "Gameplay scene must have exactly one Home-defending goal");
    }
    if (awayGoals != 1) {
        AddIssue(issues, Severity::Error, "Gameplay scene must have exactly one Away-defending goal");
    }
    if (!hasRink) {
        AddIssue(issues, Severity::Error, "Gameplay scene is missing a rink marker");
    }
    if (!hasPlayArea) {
        AddIssue(issues, Severity::Error, "Gameplay scene is missing a play area");
    }
    if (!centerFaceoff) {
        AddIssue(issues, Severity::Error, "Gameplay scene is missing a center faceoff spot");
    }

    constexpr std::array<GameplayTeam, 2> kTeams = {GameplayTeam::Home, GameplayTeam::Away};
    for (GameplayTeam team : kTeams) {
        for (int i = 0; i < 3; ++i) {
            if (!HasSpawn(registry, team, GameplayRole::Skater, i)) {
                AddIssue(issues, Severity::Error,
                         std::string("Gameplay scene is missing ") + GameplayTeamToString(team) +
                             " skater spawn " + std::to_string(i));
            }
        }
        if (!HasSpawn(registry, team, GameplayRole::Goalie, 0)) {
            AddIssue(issues, Severity::Error,
                     std::string("Gameplay scene is missing ") + GameplayTeamToString(team) + " goalie spawn");
        }
    }
}

void RegisterGameplayValidation() {
    static bool s_Registered = false;
    if (s_Registered) {
        return;
    }
    SceneValidator::RegisterExternal(ValidateGameplayScene);
    s_Registered = true;
}

} // namespace Hockey
