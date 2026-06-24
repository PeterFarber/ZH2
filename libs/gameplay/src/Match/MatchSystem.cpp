#include "Hockey/Gameplay/Match/MatchSystem.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

namespace Hockey {
namespace {

struct SpawnCandidate {
    glm::vec3 position{0.0f};
    std::filesystem::path prefabPath;
};

GameplayTeam ToGameplayTeam(Team team) {
    switch (team) {
    case Team::Home: return GameplayTeam::Home;
    case Team::Away: return GameplayTeam::Away;
    case Team::None: return GameplayTeam::None;
    }
    return GameplayTeam::None;
}

std::array<PlayerSlot, 4> SlotsForTeam(GameplayTeam team) {
    if (team == GameplayTeam::Home) {
        return {PlayerSlot::HomeSkater0, PlayerSlot::HomeSkater1, PlayerSlot::HomeSkater2, PlayerSlot::HomeGoalie};
    }
    return {PlayerSlot::AwaySkater0, PlayerSlot::AwaySkater1, PlayerSlot::AwaySkater2, PlayerSlot::AwayGoalie};
}

GameplayRole RoleForSlot(PlayerSlot slot) {
    return IsGoalieSlot(slot) ? GameplayRole::Goalie : GameplayRole::Skater;
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

Result<Entity> CreatePlayerFromSpawnPrefab(Scene& scene, const std::filesystem::path& prefabPath, PlayerSlot slot) {
    if (prefabPath.empty()) {
        return Result<Entity>::Ok(scene.CreateEntity(std::string(PlayerSlotToString(slot))));
    }

    Result<Entity> prefab = PrefabSerializer::Instantiate(scene, prefabPath);
    if (!prefab) {
        return Result<Entity>::Fail("failed to instantiate player prefab '" + prefabPath.generic_string() +
                                    "': " + prefab.error);
    }

    prefab.value.GetComponent<NameComponent>().name = std::string(PlayerSlotToString(slot));
    return prefab;
}

std::vector<SpawnCandidate> CollectSpawns(Scene& scene, Team team, bool faceoffSpawn) {
    std::vector<SpawnCandidate> spawns;
    auto view = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (spawn.team != team || spawn.faceoffSpawn != faceoffSpawn) {
            continue;
        }
        spawns.push_back({view.get<TransformComponent>(handle).localPosition, spawn.playerPrefabPath});
    }
    return spawns;
}

template <typename T> void ShuffleDeterministic(std::vector<T>& values, uint32_t seed) {
    std::mt19937 rng(seed);
    std::shuffle(values.begin(), values.end(), rng);
}

void ConfigurePlayer(Entity player, PlayerSlot slot, GameplayTeam team, const glm::vec3& position) {
    const GameplayRole role = RoleForSlot(slot);
    player.GetComponent<TransformComponent>().localPosition = position;

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
        if (player.HasComponent<SkaterComponent>()) {
            player.RemoveComponent<SkaterComponent>();
        }
        player.AddOrReplaceComponent<GoalieComponent>();
    } else {
        if (player.HasComponent<GoalieComponent>()) {
            player.RemoveComponent<GoalieComponent>();
        }
        player.AddOrReplaceComponent<SkaterComponent>();
    }

    player.AddOrReplaceComponent<PlayerRuntimeComponent>();
    player.AddOrReplaceComponent<StickComponent>().ownerPlayer = player.GetUUID();
    player.AddOrReplaceComponent<ShotComponent>();
}

Status AssignTeamPlayers(Scene& scene,
                         const GameplaySettings& settings,
                         Team markerTeam,
                         GameplayTeam gameplayTeam,
                         uint32_t seedSalt) {
    std::vector<SpawnCandidate> spawns = CollectSpawns(scene, markerTeam, false);
    constexpr std::size_t kPlayersPerTeam = 4;
    if (spawns.size() < kPlayersPerTeam) {
        return Status::Fail(std::string("not enough ") + GameplayTeamToString(gameplayTeam) + " player spawns");
    }
    ShuffleDeterministic(spawns, settings.spawnRandomSeed ^ seedSalt);

    const std::array<PlayerSlot, 4> slots = SlotsForTeam(gameplayTeam);
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const PlayerSlot slot = slots[i];
        Entity player = FindPlayerBySlot(scene, slot);
        if (!player.IsValid()) {
            Result<Entity> spawnedPlayer = CreatePlayerFromSpawnPrefab(scene, spawns[i].prefabPath, slot);
            if (!spawnedPlayer) {
                return Status::Fail(spawnedPlayer.error);
            }
            player = spawnedPlayer.value;
        }
        ConfigurePlayer(player, slot, gameplayTeam, spawns[i].position);
    }
    return Status::Ok();
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

    Status homeStatus = AssignTeamPlayers(scene, settings, Team::Home, GameplayTeam::Home, 0x1000u);
    if (!homeStatus) {
        return homeStatus;
    }
    Status awayStatus = AssignTeamPlayers(scene, settings, Team::Away, GameplayTeam::Away, 0x2000u);
    if (!awayStatus) {
        return awayStatus;
    }

    EnsurePuckGameplay(scene);
    EnsureGoalGameplay(scene);
    EnsureTeamState(scene, GameplayTeam::Home);
    EnsureTeamState(scene, GameplayTeam::Away);

    return Status::Ok();
}

} // namespace Hockey
