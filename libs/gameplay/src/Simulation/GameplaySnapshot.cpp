#include "Hockey/Gameplay/Simulation/GameplaySnapshot.hpp"

#include <algorithm>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

void FillMatch(Scene& scene, GameplaySnapshot& snapshot) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    const MatchStateComponent& match = view.get<MatchStateComponent>(*it);
    snapshot.match.phase = match.phase;
    snapshot.match.period = match.period;
    snapshot.match.periodTimeRemaining = match.periodTimeRemaining;
    snapshot.match.homeScore = match.homeScore;
    snapshot.match.awayScore = match.awayScore;
}

void FillPlayers(Scene& scene, GameplaySnapshot& snapshot) {
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        Entity entity(handle, &scene);
        const PlayerComponent& player = entity.GetComponent<PlayerComponent>();
        const TransformComponent& transform = entity.GetComponent<TransformComponent>();

        PlayerGameplaySnapshot playerSnapshot;
        playerSnapshot.entity = entity.GetUUID();
        playerSnapshot.playerIndex = player.playerIndex;
        playerSnapshot.team = player.team;
        playerSnapshot.role = player.role;
        playerSnapshot.position = transform.localPosition;
        if (entity.HasComponent<PlayerRuntimeComponent>()) {
            const PlayerRuntimeComponent& runtime = entity.GetComponent<PlayerRuntimeComponent>();
            playerSnapshot.velocity = runtime.velocity;
            playerSnapshot.facingDirection = runtime.facingDirection;
        }
        if (entity.HasComponent<SkaterComponent>()) {
            playerSnapshot.hasPuck = entity.GetComponent<SkaterComponent>().hasPuck;
        }
        snapshot.players.push_back(playerSnapshot);
    }

    std::sort(snapshot.players.begin(), snapshot.players.end(), [](const PlayerGameplaySnapshot& lhs,
                                                                    const PlayerGameplaySnapshot& rhs) {
        return lhs.playerIndex < rhs.playerIndex;
    });
}

void FillPuck(Scene& scene, GameplaySnapshot& snapshot) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    Entity puck(*it, &scene);
    snapshot.puck.entity = puck.GetUUID();
    if (puck.HasComponent<PuckGameplayComponent>()) {
        const PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
        snapshot.puck.state = gameplay.state;
        snapshot.puck.possessingPlayer = gameplay.possessingPlayer;
    }
    if (puck.HasComponent<TransformComponent>()) {
        snapshot.puck.position = puck.GetComponent<TransformComponent>().localPosition;
    }
    if (puck.HasComponent<PuckRuntimeComponent>()) {
        snapshot.puck.velocity = puck.GetComponent<PuckRuntimeComponent>().velocity;
    }
}

} // namespace

GameplaySnapshot BuildGameplaySnapshot(Scene& scene, uint64_t tick) {
    GameplaySnapshot snapshot;
    snapshot.tick = tick;
    FillMatch(scene, snapshot);
    FillPlayers(scene, snapshot);
    FillPuck(scene, snapshot);
    return snapshot;
}

} // namespace Hockey
