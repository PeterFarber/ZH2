#include "Hockey/Gameplay/Match/MatchSystem.hpp"

#include <array>
#include <filesystem>
#include <string>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

namespace Hockey {
namespace {

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

PlayerSlot SlotFor(GameplayTeam team, GameplayRole role, int index) {
    if (team == GameplayTeam::Home && role == GameplayRole::Goalie) return PlayerSlot::HomeGoalie;
    if (team == GameplayTeam::Away && role == GameplayRole::Goalie) return PlayerSlot::AwayGoalie;
    if (team == GameplayTeam::Home && index == 0) return PlayerSlot::HomeSkater0;
    if (team == GameplayTeam::Home && index == 1) return PlayerSlot::HomeSkater1;
    if (team == GameplayTeam::Home && index == 2) return PlayerSlot::HomeSkater2;
    if (team == GameplayTeam::Away && index == 0) return PlayerSlot::AwaySkater0;
    if (team == GameplayTeam::Away && index == 1) return PlayerSlot::AwaySkater1;
    if (team == GameplayTeam::Away && index == 2) return PlayerSlot::AwaySkater2;
    return PlayerSlot::None;
}

Entity FindPlayerBySlot(Scene& scene, PlayerSlot slot) {
    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity entity(handle, &scene);
        if (entity.GetComponent<PlayerComponent>().slot == slot) {
            return entity;
        }
    }
    return {};
}

Entity FindOrCreateNamed(Scene& scene, const std::string& name) {
    Entity entity = scene.FindEntityByName(name);
    return entity.IsValid() ? entity : scene.CreateEntity(name);
}

Result<Entity> CreatePlayerFromSpawnPrefab(Scene& scene, const SpawnPointComponent& spawn, PlayerSlot slot) {
    if (spawn.playerPrefabPath.empty()) {
        return Result<Entity>::Ok(scene.CreateEntity(std::string(PlayerSlotToString(slot))));
    }

    Result<Entity> prefab = PrefabSerializer::Instantiate(scene, spawn.playerPrefabPath);
    if (!prefab) {
        return Result<Entity>::Fail("failed to instantiate player prefab '" + spawn.playerPrefabPath.generic_string() +
                                    "': " + prefab.error);
    }

    prefab.value.GetComponent<NameComponent>().name = std::string(PlayerSlotToString(slot));
    return prefab;
}

void EnsureTeamState(Scene& scene, GameplayTeam team) {
    const std::string name = team == GameplayTeam::Home ? "Home Team State" : "Away Team State";
    Entity entity = FindOrCreateNamed(scene, name);
    TeamStateComponent state;
    if (entity.HasComponent<TeamStateComponent>()) {
        state = entity.GetComponent<TeamStateComponent>();
    }
    state.team = team;
    state.players.clear();

    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        if (playerComponent.team == team) {
            state.players.push_back(player.GetUUID());
            if (playerComponent.role == GameplayRole::Goalie) {
                state.goalie = player.GetUUID();
            }
        }
    }
    entity.AddOrReplaceComponent<TeamStateComponent>(state);
}

void EnsurePuckGameplay(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    Entity puck(*it, &scene);
    if (!puck.HasComponent<PuckGameplayComponent>()) {
        puck.AddComponent<PuckGameplayComponent>();
    }
    if (!puck.HasComponent<PuckRuntimeComponent>()) {
        puck.AddComponent<PuckRuntimeComponent>();
    }
}

void EnsureGoalGameplay(Scene& scene) {
    auto view = scene.Registry().view<GoalComponent>();
    for (const entt::entity handle : view) {
        Entity goal(handle, &scene);
        GoalGameplayComponent gameplay;
        if (goal.HasComponent<GoalGameplayComponent>()) {
            gameplay = goal.GetComponent<GoalGameplayComponent>();
        }
        gameplay.defendingTeam = ToGameplayTeam(goal.GetComponent<GoalComponent>().defendingTeam);
        gameplay.scoringTeam = OppositeTeam(gameplay.defendingTeam);
        goal.AddOrReplaceComponent<GoalGameplayComponent>(gameplay);
    }
}

} // namespace

Status MatchSystem::InitializeMatch(Scene& scene, const GameplaySettings& settings) {
    RegisterGameplayComponents();
    RegisterGameplayValidation();

    const auto issues = SceneValidator::Validate(scene);
    if (SceneValidator::HasErrors(issues)) {
        return Status::Fail("gameplay scene validation failed");
    }

    Entity match = FindOrCreateNamed(scene, "Match State");
    MatchStateComponent matchState;
    matchState.phase = settings.pregameCountdownSeconds > 0.0f ? MatchPhase::Warmup : MatchPhase::FaceoffSetup;
    matchState.period = 1;
    matchState.periodCount = settings.periodCount;
    matchState.periodTimeRemaining = settings.periodLengthSeconds;
    matchState.phaseTimer = settings.pregameCountdownSeconds > 0.0f ? settings.pregameCountdownSeconds : 0.0f;
    matchState.homeScore = 0;
    matchState.awayScore = 0;
    matchState.clockRunning = false;
    matchState.matchInitialized = true;
    match.AddOrReplaceComponent<MatchStateComponent>(matchState);
    match.AddOrReplaceComponent<ScoreComponent>();

    auto spawnView = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : spawnView) {
        const auto& spawn = spawnView.get<SpawnPointComponent>(handle);
        const GameplayTeam team = ToGameplayTeam(spawn.team);
        const GameplayRole role = ToGameplayRole(spawn.role);
        const PlayerSlot slot = SlotFor(team, role, spawn.index);
        if (slot == PlayerSlot::None) {
            continue;
        }

        Entity player = FindPlayerBySlot(scene, slot);
        if (!player.IsValid()) {
            Result<Entity> spawnedPlayer = CreatePlayerFromSpawnPrefab(scene, spawn, slot);
            if (!spawnedPlayer) {
                return Status::Fail(spawnedPlayer.error);
            }
            player = spawnedPlayer.value;
        }
        player.GetComponent<TransformComponent>().localPosition = spawnView.get<TransformComponent>(handle).localPosition;

        PlayerComponent playerComponent;
        if (player.HasComponent<PlayerComponent>()) {
            playerComponent = player.GetComponent<PlayerComponent>();
        }
        playerComponent.playerIndex = static_cast<uint32_t>(static_cast<int>(slot));
        playerComponent.slot = slot;
        playerComponent.team = team;
        playerComponent.role = role;
        playerComponent.activeInMatch = true;
        player.AddOrReplaceComponent<PlayerComponent>(playerComponent);

        if (role == GameplayRole::Goalie) {
            player.AddOrReplaceComponent<GoalieComponent>();
        } else {
            player.AddOrReplaceComponent<SkaterComponent>();
        }
        player.AddOrReplaceComponent<PlayerRuntimeComponent>();
        player.AddOrReplaceComponent<StickComponent>().ownerPlayer = player.GetUUID();
        player.AddOrReplaceComponent<ShotComponent>();
        player.AddOrReplaceComponent<PassComponent>();
        player.AddOrReplaceComponent<CheckComponent>();
    }

    EnsurePuckGameplay(scene);
    EnsureGoalGameplay(scene);
    EnsureTeamState(scene, GameplayTeam::Home);
    EnsureTeamState(scene, GameplayTeam::Away);

    return Status::Ok();
}

} // namespace Hockey
