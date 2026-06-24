#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

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

int CountSpawns(const entt::registry& registry, Team team, bool faceoffSpawn) {
    int count = 0;
    auto view = registry.view<const SpawnPointComponent>();
    for (const entt::entity handle : view) {
        const auto& spawn = view.get<const SpawnPointComponent>(handle);
        if (spawn.team == team && spawn.faceoffSpawn == faceoffSpawn) {
            ++count;
        }
    }
    return count;
}

} // namespace

void ValidateGameplayScene(const Scene& scene, std::vector<SceneValidationIssue>& issues) {
    const entt::registry& registry = scene.Registry();

    int puckCount = 0;
    int homeGoals = 0;
    int awayGoals = 0;
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
    if (CountSpawns(registry, Team::Home, false) < 4) {
        AddIssue(issues, Severity::Error, "Gameplay scene needs at least four Home normal spawns");
    }
    if (CountSpawns(registry, Team::Away, false) < 4) {
        AddIssue(issues, Severity::Error, "Gameplay scene needs at least four Away normal spawns");
    }
    if (CountSpawns(registry, Team::None, true) < 8) {
        AddIssue(issues, Severity::Error, "Gameplay scene needs at least eight neutral faceoff spawns");
    }

    const int homePenaltyFaceoffSpawns = CountSpawns(registry, Team::Home, true);
    if (homePenaltyFaceoffSpawns > 0 && homePenaltyFaceoffSpawns < 8) {
        AddIssue(issues, Severity::Warning, "Home penalty faceoff spawn pool should contain at least eight spawns");
    }
    const int awayPenaltyFaceoffSpawns = CountSpawns(registry, Team::Away, true);
    if (awayPenaltyFaceoffSpawns > 0 && awayPenaltyFaceoffSpawns < 8) {
        AddIssue(issues, Severity::Warning, "Away penalty faceoff spawn pool should contain at least eight spawns");
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
