#include "Hockey/Gameplay/Match/ResetSystem.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

struct SpawnCandidate {
    glm::vec3 position{0.0f};
};

struct PlayerCandidate {
    entt::entity handle = entt::null;
    uint32_t playerIndex = 0;
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

Team MarkerTeamForFaceoffCause(GameplayTeam team) {
    switch (team) {
    case GameplayTeam::Home: return Team::Home;
    case GameplayTeam::Away: return Team::Away;
    case GameplayTeam::None: return Team::None;
    }
    return Team::None;
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
        spawns.push_back({view.get<TransformComponent>(handle).localPosition});
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

std::vector<PlayerCandidate> CollectPlayers(Scene& scene) {
    std::vector<PlayerCandidate> players;
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const PlayerComponent& player = view.get<PlayerComponent>(handle);
        if (player.activeInMatch) {
            players.push_back({handle, player.playerIndex});
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

uint32_t FaceoffSequence(Entity match) {
    if (!match.IsValid() || !match.HasComponent<FaceoffComponent>()) {
        return 0;
    }
    return match.GetComponent<FaceoffComponent>().spawnSequence;
}

void ResetPlayers(Scene& scene, GameplayTeam causeTeam, const GameplaySettings& settings, uint32_t spawnSequence) {
    std::vector<SpawnCandidate> spawns = SelectFaceoffPool(scene, causeTeam);
    std::vector<PlayerCandidate> players = CollectPlayers(scene);
    if (spawns.size() < players.size()) {
        return;
    }

    ShuffleDeterministic(spawns, settings.spawnRandomSeed ^ 0x3000u ^ spawnSequence);
    ShuffleDeterministic(players, settings.spawnRandomSeed ^ 0x4000u ^ spawnSequence);

    for (std::size_t i = 0; i < players.size(); ++i) {
        Entity player(players[i].handle, &scene);
        player.GetComponent<TransformComponent>().localPosition = spawns[i].position;

        if (player.HasComponent<PlayerRuntimeComponent>()) {
            PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
            runtime.velocity = glm::vec3{0.0f};
            runtime.inputEnabled = true;
            runtime.movementEnabled = true;
        }
        if (player.HasComponent<SkaterComponent>()) {
            player.GetComponent<SkaterComponent>().hasPuck = false;
        }
    }
}

void ResetPuck(Scene& scene, GameplayTeam causeTeam) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid()) {
        return;
    }

    std::vector<SpawnCandidate> spawns = SelectFaceoffPool(scene, causeTeam);
    if (!spawns.empty()) {
        puck.GetComponent<TransformComponent>().localPosition = ComputePoolCenter(spawns);
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
}

} // namespace

void ResetSystem::BeginReset(Scene& scene, GameplayEventQueue& events, GameplayTeam causeTeam) {
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
    match.AddOrReplaceComponent<FaceoffComponent>(faceoff);

    events.Push({GameplayEventType::ResetStarted});
}

void ResetSystem::CompleteReset(Scene& scene,
                                GameplayEventQueue& events,
                                GameplayTeam causeTeam,
                                const GameplaySettings& settings) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    ResetPlayers(scene, causeTeam, settings, FaceoffSequence(match));
    ResetPuck(scene, causeTeam);

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    state.phase = MatchPhase::FaceoffSetup;
    state.phaseTimer = 0.0f;
    state.clockRunning = false;
    events.Push({GameplayEventType::ResetCompleted});
}

void ResetSystem::FixedUpdate(Scene& scene,
                              float fixedDeltaSeconds,
                              const GameplaySettings& settings,
                              GameplayEventQueue& events) {
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
        CompleteReset(scene, events, causeTeam, settings);
    }
}

} // namespace Hockey
