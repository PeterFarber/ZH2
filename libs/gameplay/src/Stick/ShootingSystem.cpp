#include "Hockey/Gameplay/Stick/ShootingSystem.hpp"

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {
namespace {

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

glm::vec3 DirectionFromInputOrFacing(Entity player, const GameplayInputFrame& input) {
    if (glm::dot(input.aim, input.aim) > 0.0001f) {
        return glm::normalize(glm::vec3{input.aim.x, 0.0f, input.aim.y});
    }
    if (player.HasComponent<PlayerRuntimeComponent>()) {
        const glm::vec3 facing = player.GetComponent<PlayerRuntimeComponent>().facingDirection;
        if (glm::length(facing) > 0.0001f) {
            return glm::normalize(facing);
        }
    }
    return {0.0f, 0.0f, 1.0f};
}

void SyncShotPuckBody(Entity puck, PhysicsWorld* physicsWorld, const glm::vec3& velocity) {
    if (physicsWorld == nullptr || !physicsWorld->IsInitialized() || !puck.IsValid() ||
        !puck.HasComponent<TransformComponent>() || !physicsWorld->HasBody(puck)) {
        return;
    }

    const TransformComponent& transform = puck.GetComponent<TransformComponent>();
    physicsWorld->SetBodyTransform(puck,
                                   transform.localPosition,
                                   glm::quat(glm::radians(transform.localRotation)));
    physicsWorld->SetLinearVelocity(puck, velocity);
}

} // namespace

void ShootingSystem::FixedUpdate(Scene& scene,
                                 const GameplayInputBuffer& inputs,
                                 const GameplayTuning& tuning,
                                 float fixedDeltaSeconds,
                                 GameplayEventQueue& events,
                                 PhysicsWorld* physicsWorld) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>() || !puck.HasComponent<PuckRuntimeComponent>()) {
        return;
    }

    PuckGameplayComponent& puckGameplay = puck.GetComponent<PuckGameplayComponent>();
    if (puckGameplay.state != PuckState::Possessed || !puckGameplay.possessingPlayer.IsValid()) {
        return;
    }

    Entity player = scene.FindEntityByUUID(puckGameplay.possessingPlayer);
    if (!player.IsValid() || !player.HasComponent<PlayerComponent>() || !player.HasComponent<ShotComponent>() ||
        !player.HasComponent<StickComponent>() || !player.GetComponent<StickComponent>().canShoot) {
        return;
    }

    GameplayInputFrame input;
    if (!inputs.GetInput(player.GetComponent<PlayerComponent>().playerIndex, input)) {
        return;
    }

    ShotComponent& shot = player.GetComponent<ShotComponent>();
    if (input.shootPressed || input.shootHeld) {
        shot.charging = true;
        shot.charge = std::min(tuning.shot.chargeSeconds, shot.charge + fixedDeltaSeconds);
    }

    if (!input.shootReleased || !shot.charging) {
        return;
    }

    const float chargeRatio = tuning.shot.chargeSeconds > 0.0f ? std::clamp(shot.charge / tuning.shot.chargeSeconds, 0.0f, 1.0f) : 1.0f;
    const float power = tuning.shot.minPower + (tuning.shot.maxPower - tuning.shot.minPower) * chargeRatio;
    const glm::vec3 direction = DirectionFromInputOrFacing(player, input);

    PuckPossession::Release(scene, puck, events);
    puckGameplay.state = PuckState::Shot;
    puckGameplay.lastTouchedPlayer = player.GetUUID();
    puckGameplay.lastTouchedTeam = player.GetComponent<PlayerComponent>().team;
    puckGameplay.timeSinceLastTouch = 0.0f;
    puckGameplay.shotIgnorePlayer = player.GetUUID();
    puckGameplay.shotIgnoreTimer = std::max(0.0f, tuning.shot.selfCollisionGraceSeconds);
    puck.GetComponent<PuckRuntimeComponent>().velocity = direction * power;
    SyncShotPuckBody(puck, physicsWorld, puck.GetComponent<PuckRuntimeComponent>().velocity);

    shot.charge = 0.0f;
    shot.charging = false;

    events.Push({GameplayEventType::PuckShot, puck.GetUUID(), player.GetUUID(), puckGameplay.lastTouchedTeam,
                 puck.HasComponent<TransformComponent>() ? puck.GetComponent<TransformComponent>().localPosition : glm::vec3{0.0f}});
}

} // namespace Hockey
