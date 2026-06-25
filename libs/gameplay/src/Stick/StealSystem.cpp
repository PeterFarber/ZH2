#include "Hockey/Gameplay/Stick/StealSystem.hpp"

#include <algorithm>

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

void ClearSkaterPossession(Scene& scene) {
    auto view = scene.Registry().view<SkaterComponent>();
    for (const entt::entity handle : view) {
        view.get<SkaterComponent>(handle).hasPuck = false;
    }
}

bool PlayerPossessesPuck(Entity player, const PuckGameplayComponent& puck) {
    return player.IsValid() && puck.possessingPlayer == player.GetUUID();
}

void SyncPuckBody(Entity puck, PhysicsWorld* physicsWorld) {
    if (physicsWorld == nullptr || !physicsWorld->IsInitialized() || !puck.IsValid() ||
        !puck.HasComponent<TransformComponent>() || !physicsWorld->HasBody(puck)) {
        return;
    }

    const TransformComponent& transform = puck.GetComponent<TransformComponent>();
    physicsWorld->SetBodyTransform(puck,
                                   transform.localPosition,
                                   glm::quat(glm::radians(transform.localRotation)));
    physicsWorld->SetLinearVelocity(puck, glm::vec3{0.0f});
}

void TransferPossession(Scene& scene,
                        Entity stealer,
                        Entity puck,
                        float puckFloorY,
                        GameplayEventQueue& events,
                        PhysicsWorld* physicsWorld) {
    PuckGameplayComponent& puckGameplay = puck.GetComponent<PuckGameplayComponent>();
    const PlayerComponent& player = stealer.GetComponent<PlayerComponent>();

    ClearSkaterPossession(scene);
    puckGameplay.state = PuckState::Possessed;
    puckGameplay.possessingPlayer = stealer.GetUUID();
    puckGameplay.lastTouchedPlayer = stealer.GetUUID();
    puckGameplay.lastTouchedTeam = player.team;
    puckGameplay.timeSinceLastTouch = 0.0f;
    puckGameplay.shotIgnorePlayer = UUID(0);
    puckGameplay.shotIgnoreTimer = 0.0f;

    if (stealer.HasComponent<SkaterComponent>()) {
        stealer.GetComponent<SkaterComponent>().hasPuck = true;
    }
    if (stealer.HasComponent<PlayerRuntimeComponent>()) {
        stealer.GetComponent<PlayerRuntimeComponent>().shotBlockedUntilRelease = true;
    }
    if (stealer.HasComponent<ShotComponent>()) {
        ShotComponent& shot = stealer.GetComponent<ShotComponent>();
        shot.charge = 0.0f;
        shot.charging = false;
    }
    if (puck.HasComponent<TransformComponent>()) {
        glm::vec3 puckPosition = StickHandling::GetStickWorldPosition(stealer);
        puckPosition.y = puckFloorY;
        puck.GetComponent<TransformComponent>().localPosition = puckPosition;
    }
    if (puck.HasComponent<PuckRuntimeComponent>()) {
        PuckRuntimeComponent& runtime = puck.GetComponent<PuckRuntimeComponent>();
        runtime.velocity = glm::vec3{0.0f};
        runtime.targetPosition =
            puck.HasComponent<TransformComponent>() ? puck.GetComponent<TransformComponent>().localPosition : glm::vec3{0.0f};
    }
    SyncPuckBody(puck, physicsWorld);

    events.Push({GameplayEventType::PuckPossessionChanged,
                 puck.GetUUID(),
                 stealer.GetUUID(),
                 player.team,
                 puck.HasComponent<TransformComponent>() ? puck.GetComponent<TransformComponent>().localPosition
                                                        : glm::vec3{0.0f}});
}

} // namespace

void StealSystem::FixedUpdate(Scene& scene,
                              const GameplayInputBuffer& inputs,
                              const GameplayTuning& tuning,
                              float fixedDeltaSeconds,
                              GameplayEventQueue& events,
                              PhysicsWorld* physicsWorld) {
    Entity puck = FindPuck(scene);
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, StickComponent, TransformComponent>();

    for (const entt::entity handle : view) {
        Entity stealer(handle, &scene);
        PlayerRuntimeComponent& runtime = stealer.GetComponent<PlayerRuntimeComponent>();
        runtime.stealCooldown = std::max(0.0f, runtime.stealCooldown - fixedDeltaSeconds);

        GameplayInputFrame input;
        if (!inputs.GetInput(stealer.GetComponent<PlayerComponent>().playerIndex, input) || !runtime.inputEnabled ||
            !input.stealPressed || runtime.stealCooldown > 0.0f) {
            continue;
        }

        runtime.stealCooldown = tuning.skater.stealCooldownSeconds;
        events.Push({GameplayEventType::StealAttempted,
                     stealer.GetUUID(),
                     puck.IsValid() ? puck.GetUUID() : UUID(0),
                     stealer.GetComponent<PlayerComponent>().team,
                     stealer.GetComponent<TransformComponent>().localPosition});

        if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>() || PlayerPossessesPuck(stealer, puck.GetComponent<PuckGameplayComponent>()) ||
            puck.GetComponent<PuckGameplayComponent>().state != PuckState::Possessed ||
            !puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid() || !StickHandling::CanControlPuck(stealer, puck)) {
            continue;
        }

        TransferPossession(scene, stealer, puck, tuning.puck.floorY, events, physicsWorld);
    }
}

} // namespace Hockey
