#include "Hockey/Gameplay/Match/ResetSystem.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

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

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindCenterFaceoff(Scene& scene) {
    auto view = scene.Registry().view<FaceoffSpotComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        if (view.get<FaceoffSpotComponent>(handle).index == 0) {
            return Entity(handle, &scene);
        }
    }
    return {};
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindSpawnForPlayer(Scene& scene, const PlayerComponent& player) {
    auto view = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (ToGameplayTeam(spawn.team) != player.team || ToGameplayRole(spawn.role) != player.role) {
            continue;
        }
        if (player.role == GameplayRole::Goalie || spawn.index == static_cast<int>(player.playerIndex % 4u)) {
            return Entity(handle, &scene);
        }
    }
    return {};
}

void ResetPlayers(Scene& scene) {
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        Entity spawn = FindSpawnForPlayer(scene, playerComponent);
        if (spawn.IsValid()) {
            player.GetComponent<TransformComponent>().localPosition = spawn.GetComponent<TransformComponent>().localPosition;
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
    }
}

void ResetPuck(Scene& scene) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid()) {
        return;
    }

    Entity faceoff = FindCenterFaceoff(scene);
    if (faceoff.IsValid()) {
        puck.GetComponent<TransformComponent>().localPosition = faceoff.GetComponent<TransformComponent>().localPosition;
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

void ResetSystem::BeginReset(Scene& scene, GameplayEventQueue& events) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    MatchStateComponent& state = match.GetComponent<MatchStateComponent>();
    state.phase = MatchPhase::GoalScored;
    state.phaseTimer = 0.0f;
    state.clockRunning = false;
    events.Push({GameplayEventType::ResetStarted});
}

void ResetSystem::CompleteReset(Scene& scene, GameplayEventQueue& events) {
    Entity match = FindMatch(scene);
    if (!match.IsValid()) {
        return;
    }

    ResetPlayers(scene);
    ResetPuck(scene);

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
        CompleteReset(scene, events);
    }
}

} // namespace Hockey
