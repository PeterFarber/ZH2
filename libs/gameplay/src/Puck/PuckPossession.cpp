#include "Hockey/Gameplay/Puck/PuckPossession.hpp"

#include <algorithm>
#include <cmath>

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

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
    return state == PuckState::Loose || state == PuckState::Shot;
}

glm::vec2 Xz(const glm::vec3& value) {
    return {value.x, value.z};
}

glm::vec3 AbsScale(const glm::vec3& scale) {
    return {std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)};
}

glm::vec3 ScaleOffset(const glm::vec3& offset, const glm::vec3& scale) {
    return {offset.x * scale.x, offset.y * scale.y, offset.z * scale.z};
}

float XzRadiusScale(const glm::vec3& scale) {
    return std::max(scale.x, scale.z);
}

float BoxFootprintRadius(const glm::vec3& halfExtents, const glm::vec3& scale) {
    return std::max(halfExtents.x * scale.x, halfExtents.z * scale.z);
}

template <typename Func> bool VisitColliderFootprints(Entity entity, Func&& func) {
    if (!entity.IsValid() || !entity.HasComponent<TransformComponent>()) {
        return false;
    }

    const TransformComponent& transform = entity.GetComponent<TransformComponent>();
    const glm::vec3 basePosition = transform.localPosition;
    const glm::vec3 scale = AbsScale(transform.localScale);
    const float radiusScale = XzRadiusScale(scale);
    bool visited = false;
    bool stop = false;

    auto visit = [&](const glm::vec3& offset, float radius) {
        if (stop || radius <= 0.0f) {
            return;
        }
        visited = true;
        stop = func(Xz(basePosition + ScaleOffset(offset, scale)), radius);
    };

    if (entity.HasComponent<SphereColliderComponent>()) {
        const SphereColliderComponent& collider = entity.GetComponent<SphereColliderComponent>();
        visit(collider.offset, collider.radius * radiusScale);
    }
    if (entity.HasComponent<CapsuleColliderComponent>()) {
        const CapsuleColliderComponent& collider = entity.GetComponent<CapsuleColliderComponent>();
        visit(collider.offset, collider.radius * radiusScale);
    }
    if (entity.HasComponent<CylinderColliderComponent>()) {
        const CylinderColliderComponent& collider = entity.GetComponent<CylinderColliderComponent>();
        visit(collider.offset, collider.radius * radiusScale);
    }
    if (entity.HasComponent<BoxColliderComponent>()) {
        const BoxColliderComponent& collider = entity.GetComponent<BoxColliderComponent>();
        visit(collider.offset, BoxFootprintRadius(collider.halfExtents, scale));
    }

    return visited;
}

bool FootprintsOverlap(const glm::vec2& aCenter, float aRadius, const glm::vec2& bCenter, float bRadius) {
    const glm::vec2 delta = aCenter - bCenter;
    const float radius = aRadius + bRadius;
    return delta.x * delta.x + delta.y * delta.y <= radius * radius;
}

bool BodyContactsPuck(Entity player, Entity puck) {
    bool contact = false;
    VisitColliderFootprints(player, [&](const glm::vec2& playerCenter, float playerRadius) {
        VisitColliderFootprints(puck, [&](const glm::vec2& puckCenter, float puckRadius) {
            contact = FootprintsOverlap(playerCenter, playerRadius, puckCenter, puckRadius);
            return contact;
        });
        return contact;
    });
    return contact;
}

bool CanAcquireByContact(Entity player, Entity puck) {
    return StickHandling::CanControlPuck(player, puck) || BodyContactsPuck(player, puck);
}

bool IsTemporarilyIgnoredShooter(Entity player, const PuckGameplayComponent& gameplay) {
    return gameplay.shotIgnoreTimer > 0.0f && gameplay.shotIgnorePlayer.IsValid() &&
           gameplay.shotIgnorePlayer == player.GetUUID();
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

void FollowPossessingPlayer(Scene& scene,
                            Entity puck,
                            const PuckGameplayComponent& gameplay,
                            PhysicsWorld* physicsWorld) {
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
    SyncPuckBody(puck, physicsWorld);
}

} // namespace

bool PuckPossession::TryAcquire(Scene& scene,
                                Entity player,
                                Entity puck,
                                GameplayEventQueue& events,
                                PhysicsWorld* physicsWorld) {
    if (!player.IsValid() || !puck.IsValid() || !player.HasComponent<PlayerComponent>() ||
        !puck.HasComponent<PuckGameplayComponent>() || !CanAcquireByContact(player, puck)) {
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

    FollowPossessingPlayer(scene, puck, gameplay, physicsWorld);
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

void PuckPossession::FixedUpdate(Scene& scene, GameplayEventQueue& events, PhysicsWorld* physicsWorld) {
    Entity puck = FindPuck(scene);
    if (!puck.IsValid() || !puck.HasComponent<PuckGameplayComponent>()) {
        return;
    }

    PuckGameplayComponent& gameplay = puck.GetComponent<PuckGameplayComponent>();
    if (gameplay.state == PuckState::Possessed && gameplay.possessingPlayer.IsValid()) {
        FollowPossessingPlayer(scene, puck, gameplay, physicsWorld);
        return;
    }

    if (!CanAcquireState(gameplay.state)) {
        return;
    }

    auto players = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : players) {
        Entity player(handle, &scene);
        if (TryAcquire(scene, player, puck, events, physicsWorld)) {
            return;
        }
    }
}

} // namespace Hockey
