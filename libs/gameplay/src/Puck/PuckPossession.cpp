#include "Hockey/Gameplay/Puck/PuckPossession.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindEntity(Scene& scene, UUID id) {
    return id.IsValid() ? scene.FindEntityByUUID(id) : Entity{};
}

void ClearSkaterPossession(Scene& scene) {
    auto view = scene.Registry().view<SkaterComponent>();
    for (const entt::entity handle : view) {
        view.get<SkaterComponent>(handle).hasPuck = false;
    }
}

bool CanAcquireState(PuckState state) {
    return state == PuckState::Loose || state == PuckState::Shot || state == PuckState::Passed;
}

bool IsTemporarilyIgnoredShooter(Entity player, const PuckGameplayComponent& gameplay) {
    return gameplay.shotIgnoreTimer > 0.0f && gameplay.shotIgnorePlayer.IsValid() &&
           gameplay.shotIgnorePlayer == player.GetUUID();
}

void FollowPossessingPlayer(Scene& scene, Entity puck, const PuckGameplayComponent& gameplay) {
    Entity player = FindEntity(scene, gameplay.possessingPlayer);
    if (!player.IsValid() || !player.HasComponent<TransformComponent>() || !puck.HasComponent<TransformComponent>()) {
        return;
    }

    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(player);
    if (puck.HasComponent<PuckRuntimeComponent>()) {
        PuckRuntimeComponent& runtime = puck.GetComponent<PuckRuntimeComponent>();
        runtime.velocity = glm::vec3{0.0f};
        runtime.targetPosition = puck.GetComponent<TransformComponent>().localPosition;
    }
}

} // namespace

bool PuckPossession::TryAcquire(Scene& scene, Entity player, Entity puck, GameplayEventQueue& events) {
    if (!player.IsValid() || !puck.IsValid() || !player.HasComponent<PlayerComponent>() ||
        !puck.HasComponent<PuckGameplayComponent>() || !StickHandling::CanControlPuck(player, puck)) {
        return false;
    }

    PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
    if (!CanAcquireState(gameplay.state) || gameplay.possessingPlayer.IsValid()) {
        return false;
    }
    if (IsTemporarilyIgnoredShooter(player, gameplay)) {
        return false;
    }

    ClearSkaterPossession(scene);
    const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
    gameplay.state = PuckState::Possessed;
    gameplay.possessingPlayer = player.GetUUID();
    gameplay.lastTouchedPlayer = player.GetUUID();
    gameplay.lastTouchedTeam = playerComponent.team;
    gameplay.timeSinceLastTouch = 0.0f;
    gameplay.shotIgnorePlayer = UUID(0);
    gameplay.shotIgnoreTimer = 0.0f;

    if (player.HasComponent<SkaterComponent>()) {
        player.GetComponent<SkaterComponent>().hasPuck = true;
    }

    FollowPossessingPlayer(scene, puck, gameplay);
    events.Push({GameplayEventType::PuckPossessionChanged, puck.GetUUID(), player.GetUUID(), playerComponent.team,
                 puck.GetComponent<TransformComponent>().localPosition});
    return true;
}

void PuckPossession::Release(Scene& scene, Entity puck, GameplayEventQueue& events) {
    if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>()) {
        return;
    }

    PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
    const UUID previousPlayer = gameplay.possessingPlayer;
    if (!previousPlayer.IsValid() && gameplay.state == PuckState::Loose) {
        return;
    }

    GameplayTeam previousTeam = gameplay.lastTouchedTeam;
    Entity player = FindEntity(scene, previousPlayer);
    if (player.IsValid() && player.HasComponent<PlayerComponent>()) {
        previousTeam = player.GetComponent<PlayerComponent>().team;
    }

    ClearSkaterPossession(scene);
    gameplay.state = PuckState::Loose;
    gameplay.possessingPlayer = UUID(0);

    events.Push({GameplayEventType::PuckPossessionChanged, puck.GetUUID(), previousPlayer, previousTeam,
                 puck.HasComponent<TransformComponent>() ? puck.GetComponent<TransformComponent>().localPosition : glm::vec3{0.0f}});
}

void PuckPossession::FixedUpdate(Scene& scene, GameplayEventQueue& events) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>()) {
        return;
    }

    PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
    if (gameplay.state == PuckState::Possessed && gameplay.possessingPlayer.IsValid()) {
        FollowPossessingPlayer(scene, puck, gameplay);
        return;
    }

    if (!CanAcquireState(gameplay.state)) {
        return;
    }

    auto players = scene.Registry().view<PlayerComponent, StickComponent>();
    for (const entt::entity handle : players) {
        Entity player(handle, &scene);
        if (TryAcquire(scene, player, puck, events)) {
            return;
        }
    }
}

} // namespace Hockey
