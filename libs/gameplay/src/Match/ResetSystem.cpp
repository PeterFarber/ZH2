#include "Hockey/Gameplay/Match/ResetSystem.hpp"

#include <algorithm>
#include <optional>
#include <random>
#include <vector>

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/Transform.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Player/PlayerFacing.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {
namespace {

struct SpawnCandidate {
    glm::vec3 position{0.0f};
    Team assignedTeam = Team::None;
    PlayerRole assignedRole = PlayerRole::Skater;
    bool hasAssignedTeam = false;
    bool hasAssignedRole = false;
};

struct PlayerCandidate {
    entt::entity handle = entt::null;
    uint32_t playerIndex = 0;
    GameplayTeam team = GameplayTeam::None;
    GameplayRole role = GameplayRole::Skater;
};

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

std::optional<glm::vec3> FindPuckPosition(Scene& scene) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<TransformComponent>()) {
        return std::nullopt;
    }
    return puck.GetComponent<TransformComponent>().localPosition;
}

Team MarkerTeamForFaceoffCause(GameplayTeam team) {
    switch (team) {
    case GameplayTeam::Home: return Team::Home;
    case GameplayTeam::Away: return Team::Away;
    case GameplayTeam::None: return Team::None;
    }
    return Team::None;
}

PlayerRole MarkerRoleForPlayer(GameplayRole role) {
    return role == GameplayRole::Goalie ? PlayerRole::Goalie : PlayerRole::Skater;
}

SpawnCandidate MakeSpawnCandidate(Scene& scene,
                                  entt::entity handle,
                                  const SpawnPointComponent& spawn,
                                  const TransformComponent& transform) {
    SpawnCandidate candidate;
    candidate.position = transform.localPosition;

    if (const auto* team = scene.Registry().try_get<TeamComponent>(handle)) {
        candidate.assignedTeam = team->team;
        candidate.hasAssignedTeam = team->team != Team::None;
    } else if (!spawn.faceoffSpawn && spawn.team != Team::None) {
        candidate.assignedTeam = spawn.team;
        candidate.hasAssignedTeam = true;
    }

    if (const auto* role = scene.Registry().try_get<PlayerRoleComponent>(handle)) {
        candidate.assignedRole = role->role;
        candidate.hasAssignedRole = true;
    }

    return candidate;
}

std::vector<SpawnCandidate> CollectFaceoffSpawns(Scene& scene, GameplayTeam causeTeam) {
    std::vector<SpawnCandidate> spawns;
    const Team markerTeam = MarkerTeamForFaceoffCause(causeTeam);
    auto view = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (spawn.team != markerTeam || !spawn.faceoffSpawn) {
            continue;
        }
        spawns.push_back(MakeSpawnCandidate(scene, handle, spawn, view.get<TransformComponent>(handle)));
    }
    return spawns;
}

std::vector<SpawnCandidate> CollectNormalSpawns(Scene& scene) {
    std::vector<SpawnCandidate> spawns;
    auto view = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (spawn.faceoffSpawn || spawn.team == Team::None) {
            continue;
        }
        spawns.push_back(MakeSpawnCandidate(scene, handle, spawn, view.get<TransformComponent>(handle)));
    }
    return spawns;
}

std::vector<SpawnCandidate> SelectFaceoffPool(Scene& scene, GameplayTeam causeTeam) {
    std::vector<SpawnCandidate> spawns = CollectFaceoffSpawns(scene, causeTeam);
    if (causeTeam == GameplayTeam::None || spawns.size() >= 8) {
        return spawns;
    }
    return CollectFaceoffSpawns(scene, GameplayTeam::None);
}

std::vector<SpawnCandidate> SelectSpawnPool(Scene& scene, GameplayTeam causeTeam, bool useNormalSpawns) {
    return useNormalSpawns ? CollectNormalSpawns(scene) : SelectFaceoffPool(scene, causeTeam);
}

std::vector<PlayerCandidate> CollectPlayers(Scene& scene) {
    std::vector<PlayerCandidate> players;
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const PlayerComponent& player = view.get<PlayerComponent>(handle);
        if (player.activeInMatch) {
            players.push_back({handle, player.playerIndex, player.team, player.role});
        }
    }
    std::sort(players.begin(), players.end(), [](const PlayerCandidate& lhs, const PlayerCandidate& rhs) {
        return lhs.playerIndex < rhs.playerIndex;
    });
    return players;
}

template <typename T> void ShuffleDeterministic(std::vector<T>& values, uint32_t seed) {
    std::mt19937 rng(seed);
    std::shuffle(values.begin(), values.end(), rng);
}

glm::vec3 ComputePoolCenter(const std::vector<SpawnCandidate>& spawns) {
    if (spawns.empty()) {
        return glm::vec3{0.0f};
    }
    glm::vec3 center{0.0f};
    for (const SpawnCandidate& spawn : spawns) {
        center += spawn.position;
    }
    return center / static_cast<float>(spawns.size());
}

std::size_t FindMatchingSpawn(const std::vector<SpawnCandidate>& spawns,
                              const std::vector<bool>& used,
                              const PlayerCandidate& player) {
    const Team playerTeam = MarkerTeamForFaceoffCause(player.team);
    const PlayerRole playerRole = MarkerRoleForPlayer(player.role);

    const auto find = [&](auto&& predicate) {
        for (std::size_t i = 0; i < spawns.size(); ++i) {
            if (!used[i] && predicate(spawns[i])) {
                return i;
            }
        }
        return spawns.size();
    };

    std::size_t match = find([&](const SpawnCandidate& spawn) {
        return spawn.hasAssignedTeam && spawn.assignedTeam == playerTeam && spawn.hasAssignedRole &&
               spawn.assignedRole == playerRole;
    });
    if (match != spawns.size()) {
        return match;
    }

    match = find([&](const SpawnCandidate& spawn) {
        return spawn.hasAssignedTeam && spawn.assignedTeam == playerTeam && !spawn.hasAssignedRole;
    });
    if (match != spawns.size()) {
        return match;
    }

    match = find([&](const SpawnCandidate& spawn) {
        return spawn.hasAssignedTeam && spawn.assignedTeam == playerTeam;
    });
    if (match != spawns.size()) {
        return match;
    }

    match = find([](const SpawnCandidate& spawn) { return !spawn.hasAssignedTeam; });
    if (match != spawns.size()) {
        return match;
    }

    return find([](const SpawnCandidate&) { return true; });
}

uint32_t FaceoffSequence(Entity match) {
    if (!match.IsValid() || !match.HasComponent<FaceoffComponent>()) {
        return 0;
    }
    return match.GetComponent<FaceoffComponent>().spawnSequence;
}

void SyncBodyToTransform(Scene& scene, Entity entity, PhysicsWorld* physicsWorld) {
    if (physicsWorld == nullptr || !physicsWorld->IsInitialized() || !physicsWorld->HasBody(entity) ||
        !entity.HasComponent<TransformComponent>()) {
        return;
    }

    glm::vec3 position{0.0f};
    glm::vec3 rotationDegrees{0.0f};
    glm::vec3 scale{1.0f};
    DecomposeTransform(scene.GetWorldTransform(entity), position, rotationDegrees, scale);
    physicsWorld->SetBodyTransform(entity, position, glm::quat(glm::radians(rotationDegrees)));
    physicsWorld->SetLinearVelocity(entity, glm::vec3{0.0f});
}

void ResetPlayers(Scene& scene,
                  GameplayTeam causeTeam,
                  bool useNormalSpawns,
                  const GameplaySettings& settings,
                  uint32_t spawnSequence,
                  const std::optional<glm::vec3>& puckPosition,
                  PhysicsWorld* physicsWorld) {
    std::vector<SpawnCandidate> spawns = SelectSpawnPool(scene, causeTeam, useNormalSpawns);
    std::vector<PlayerCandidate> players = CollectPlayers(scene);
    if (spawns.size() < players.size()) {
        return;
    }

    ShuffleDeterministic(spawns, settings.spawnRandomSeed ^ 0x3000u ^ spawnSequence);
    ShuffleDeterministic(players, settings.spawnRandomSeed ^ 0x4000u ^ spawnSequence);
    std::vector<bool> usedSpawns(spawns.size(), false);

    for (std::size_t i = 0; i < players.size(); ++i) {
        const std::size_t spawnIndex = FindMatchingSpawn(spawns, usedSpawns, players[i]);
        if (spawnIndex == spawns.size()) {
            return;
        }
        usedSpawns[spawnIndex] = true;

        Entity player(players[i].handle, &scene);
        player.GetComponent<TransformComponent>().localPosition = spawns[spawnIndex].position;
        if (puckPosition.has_value()) {
            FacePlayerToward(player, *puckPosition);
        }

        if (player.HasComponent<PlayerRuntimeComponent>()) {
            PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
            runtime.velocity = glm::vec3{0.0f};
            runtime.inputEnabled = true;
            runtime.movementEnabled = true;
        }
        if (player.HasComponent<SkaterComponent>()) {
            player.GetComponent<SkaterComponent>().hasPuck = false;
        }
        SyncBodyToTransform(scene, player, physicsWorld);
    }
}

bool FindNormalPuckResetPosition(Scene& scene, glm::vec3& outPosition) {
    auto outOfPlayView = scene.Registry().view<OutOfPlayComponent>();
    const auto outOfPlayIt = outOfPlayView.begin();
    if (outOfPlayIt != outOfPlayView.end()) {
        outPosition = outOfPlayView.get<OutOfPlayComponent>(*outOfPlayIt).resetPosition;
        return true;
    }

    std::vector<SpawnCandidate> normalSpawns = CollectNormalSpawns(scene);
    if (normalSpawns.empty()) {
        return false;
    }

    outPosition = ComputePoolCenter(normalSpawns);
    return true;
}

void ResetPuck(Scene& scene, GameplayTeam causeTeam, bool useNormalSpawns, PhysicsWorld* physicsWorld) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid()) {
        return;
    }

    glm::vec3 resetPosition{0.0f};
    if (useNormalSpawns) {
        if (FindNormalPuckResetPosition(scene, resetPosition)) {
            puck.GetComponent<TransformComponent>().localPosition = resetPosition;
        }
    } else {
        std::vector<SpawnCandidate> spawns = SelectFaceoffPool(scene, causeTeam);
        if (!spawns.empty()) {
            puck.GetComponent<TransformComponent>().localPosition = ComputePoolCenter(spawns);
        }
    }

    if (puck.HasComponent<PuckGameplayComponent>()) {
        PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
        gameplay.state = PuckState::Loose;
        gameplay.possessingPlayer = UUID(0);
        gameplay.timeSinceLastTouch = 0.0f;
        gameplay.inPlay = true;
    }
    if (puck.HasComponent<PuckRuntimeComponent>()) {
        PuckRuntimeComponent& runtime = puck.GetComponent<PuckRuntimeComponent>();
        runtime.velocity = glm::vec3{0.0f};
        runtime.targetPosition = puck.GetComponent<TransformComponent>().localPosition;
    }
    SyncBodyToTransform(scene, puck, physicsWorld);
}

} // namespace

void ResetSystem::BeginReset(Scene& scene,
                             GameplayEventQueue& events,
                             GameplayTeam causeTeam,
                             bool useNormalSpawnsForReset) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    state.phase = MatchPhase::GoalScored;
    state.phaseTimer = 0.0f;
    state.clockRunning = false;

    FaceoffComponent faceoff;
    if (match.HasComponent<FaceoffComponent>()) {
        faceoff = match.GetComponent<FaceoffComponent>();
    }
    faceoff.causeTeam = causeTeam;
    ++faceoff.spawnSequence;
    faceoff.timer = 0.0f;
    faceoff.locked = false;
    faceoff.useNormalSpawnsForReset = useNormalSpawnsForReset;
    match.AddOrReplaceComponent<FaceoffComponent>(faceoff);

    events.Push({GameplayEventType::ResetStarted});
}

void ResetSystem::CompleteReset(Scene& scene,
                                GameplayEventQueue& events,
                                GameplayTeam causeTeam,
                                bool useNormalSpawnsForReset,
                                const GameplaySettings& settings,
                                PhysicsWorld* physicsWorld) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    ResetPuck(scene, causeTeam, useNormalSpawnsForReset, physicsWorld);
    ResetPlayers(scene,
                 causeTeam,
                 useNormalSpawnsForReset,
                 settings,
                 FaceoffSequence(match),
                 FindPuckPosition(scene),
                 physicsWorld);

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    state.phase = MatchPhase::FaceoffSetup;
    state.phaseTimer = 0.0f;
    state.clockRunning = false;
    events.Push({GameplayEventType::ResetCompleted});
}

void ResetSystem::FixedUpdate(Scene& scene,
                              float fixedDeltaSeconds,
                              const GameplaySettings& settings,
                              GameplayEventQueue& events,
                              PhysicsWorld* physicsWorld) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    if (state.phase != MatchPhase::GoalScored) {
        return;
    }

    state.phaseTimer += fixedDeltaSeconds;
    if (state.phaseTimer >= settings.postGoalDelaySeconds) {
        GameplayTeam causeTeam = GameplayTeam::None;
        if (match.HasComponent<FaceoffComponent>()) {
            causeTeam = match.GetComponent<FaceoffComponent>().causeTeam;
        }
        bool useNormalSpawnsForReset = false;
        if (match.HasComponent<FaceoffComponent>()) {
            useNormalSpawnsForReset = match.GetComponent<FaceoffComponent>().useNormalSpawnsForReset;
        }
        CompleteReset(scene, events, causeTeam, useNormalSpawnsForReset, settings, physicsWorld);
    }
}

} // namespace Hockey
