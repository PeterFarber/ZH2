#include "Hockey/Gameplay/Player/PlayerMovement.hpp"

#include <algorithm>
#include <cmath>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {
namespace {

float LengthSq(const glm::vec2& value) {
    return glm::dot(value, value);
}

glm::vec2 ClampedInput(glm::vec2 input) {
    const float lengthSq = LengthSq(input);
    if (lengthSq <= 1.0f) {
        return input;
    }
    return glm::normalize(input);
}

glm::vec3 Xz(const glm::vec2& value) {
    return {value.x, 0.0f, value.y};
}

glm::vec2 Xz(const glm::vec3& value) {
    return {value.x, value.z};
}

glm::vec3 MoveToward(const glm::vec3& current, const glm::vec3& target, float maxDelta) {
    const glm::vec3 delta = target - current;
    const float distance = glm::length(delta);
    if (distance <= maxDelta || distance <= 0.0001f) {
        return target;
    }
    return current + (delta / distance) * maxDelta;
}

bool CanMoveInCurrentMatchPhase(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return true;
    }
    return view.get<MatchStateComponent>(*it).phase == MatchPhase::Playing;
}

struct MovementConfig {
    float maxSpeed = 0.0f;
    float acceleration = 0.0f;
    float deceleration = 0.0f;
    bool lockGoalieDepth = false;
};

MovementConfig ConfigFor(Entity player, const GameplayTuning& tuning) {
    if (player.HasComponent<GoalieComponent>()) {
        const GoalieComponent& goalie = player.GetComponent<GoalieComponent>();
        return {tuning.goalie.maxSpeed,
                tuning.goalie.acceleration,
                tuning.skater.deceleration,
                goalie.lockToCrease};
    }

    return {tuning.skater.maxSpeed, tuning.skater.acceleration, tuning.skater.deceleration, false};
}

glm::vec3 NormalizedOrForward(const glm::vec3& value) {
    return glm::length(value) > 0.0001f ? glm::normalize(value) : glm::vec3{0.0f, 0.0f, 1.0f};
}

float YawDegreesFromFacing(const glm::vec3& facingDirection) {
    constexpr float kRadiansToDegrees = 57.29577951308232f;
    const glm::vec3 facing = NormalizedOrForward(facingDirection);
    return std::atan2(facing.x, facing.z) * kRadiansToDegrees;
}

void ApplyFacingRotation(const PlayerRuntimeComponent& runtime, TransformComponent& transform) {
    transform.localRotation = {0.0f, YawDegreesFromFacing(runtime.facingDirection), 0.0f};
}

void TickAbilityTimers(Entity player,
                       PlayerRuntimeComponent& runtime,
                       const GameplayTuning& tuning,
                       float fixedDeltaSeconds,
                       GameplayEventQueue& events) {
    runtime.boostCooldown = glm::max(0.0f, runtime.boostCooldown - fixedDeltaSeconds);
    runtime.shieldCooldown = glm::max(0.0f, runtime.shieldCooldown - fixedDeltaSeconds);

    if (runtime.lastBrakePressedTime >= 0.0f) {
        runtime.lastBrakePressedTime += fixedDeltaSeconds;
        if (runtime.lastBrakePressedTime > tuning.skater.doubleStopWindowSeconds) {
            runtime.lastBrakePressedTime = -1000.0f;
        }
    }

    if (player.HasComponent<GoalieComponent>()) {
        const uint32_t maxCharges = std::max(1u, tuning.goalie.boostCharges);
        runtime.goalieBoostCharges = std::min(runtime.goalieBoostCharges, maxCharges);
        if (runtime.goalieBoostCharges < maxCharges && tuning.goalie.boostRechargeSeconds > 0.0f) {
            runtime.goalieBoostRechargeTimer += fixedDeltaSeconds;
            while (runtime.goalieBoostRechargeTimer + 0.0001f >= tuning.goalie.boostRechargeSeconds &&
                   runtime.goalieBoostCharges < maxCharges) {
                runtime.goalieBoostRechargeTimer -= tuning.goalie.boostRechargeSeconds;
                ++runtime.goalieBoostCharges;
            }
        } else if (runtime.goalieBoostCharges >= maxCharges) {
            runtime.goalieBoostRechargeTimer = 0.0f;
        }

        if (runtime.shieldActive) {
            runtime.shieldTimer = glm::max(0.0f, runtime.shieldTimer - fixedDeltaSeconds);
            if (runtime.shieldTimer <= 0.0f) {
                runtime.shieldActive = false;
                events.Push({GameplayEventType::GoalieShieldEnded,
                             player.GetUUID(),
                             {},
                             player.GetComponent<PlayerComponent>().team,
                             player.GetComponent<TransformComponent>().localPosition});
            }
        }
    }
}

void ApplyGoalieShield(Scene& scene,
                       Entity goalie,
                       PlayerRuntimeComponent& goalieRuntime,
                       const GameplayTuning& tuning) {
    if (!goalieRuntime.shieldActive || !goalie.HasComponent<TransformComponent>()) {
        return;
    }

    const glm::vec3 goaliePosition = goalie.GetComponent<TransformComponent>().localPosition;
    const float radius = std::max(0.0f, tuning.goalie.shieldRadius);
    if (radius <= 0.0f) {
        return;
    }

    auto pucks = scene.Registry().view<PuckComponent, PuckGameplayComponent, PuckRuntimeComponent, TransformComponent>();
    for (const entt::entity handle : pucks) {
        Entity puck(handle, &scene);
        const glm::vec3 offset = puck.GetComponent<TransformComponent>().localPosition - goaliePosition;
        if (glm::length(offset) > radius) {
            continue;
        }

        const glm::vec3 direction = NormalizedOrForward(offset);
        puck.GetComponent<PuckRuntimeComponent>().velocity = direction * tuning.goalie.shieldReflectImpulse;
        puck.GetComponent<PuckGameplayComponent>().state = PuckState::Shot;
    }

    auto players = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();
    for (const entt::entity handle : players) {
        Entity player(handle, &scene);
        if (player == goalie) {
            continue;
        }

        const glm::vec3 offset = player.GetComponent<TransformComponent>().localPosition - goaliePosition;
        if (glm::length(offset) > radius) {
            continue;
        }

        player.GetComponent<PlayerRuntimeComponent>().velocity +=
            NormalizedOrForward(offset) * (tuning.goalie.shieldReflectImpulse * 0.35f);
    }
}

bool HasDynamicPhysicsBody(Entity player, PhysicsWorld* physicsWorld) {
    return physicsWorld != nullptr && physicsWorld->IsInitialized() && physicsWorld->HasBody(player) &&
           player.HasComponent<RigidBodyComponent>() &&
           player.GetComponent<RigidBodyComponent>().type == RigidBodyType::Dynamic;
}

} // namespace

void PlayerMovement::FixedUpdate(Scene& scene,
                                 PhysicsWorld* physicsWorld,
                                 const GameplayInputBuffer& inputs,
                                 const GameplayTuning& tuning,
                                 float fixedDeltaSeconds,
                                 GameplayEventQueue& events) {
    const bool matchAllowsMovement = CanMoveInCurrentMatchPhase(scene);
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();

    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
        TransformComponent& transform = player.GetComponent<TransformComponent>();
        TickAbilityTimers(player, runtime, tuning, fixedDeltaSeconds, events);

        GameplayInputFrame input;
        inputs.GetInput(playerComponent.playerIndex, input);
        if (input.clearMoveTarget || input.brakePressed) {
            runtime.hasMoveTarget = false;
        }
        if (input.setMoveTarget) {
            runtime.moveTarget = input.moveTarget;
            runtime.hasMoveTarget = true;
        }

        const MovementConfig config = ConfigFor(player, tuning);
        glm::vec2 moveInput = ClampedInput(input.move);
        glm::vec2 waypointDirection{0.0f};
        bool hasWaypointDirection = false;
        if (runtime.hasMoveTarget) {
            const glm::vec2 toTarget = Xz(runtime.moveTarget - transform.localPosition);
            const float distanceToTarget = glm::length(toTarget);
            if (distanceToTarget <= 0.35f) {
                runtime.hasMoveTarget = false;
                moveInput = {0.0f, 0.0f};
            } else {
                waypointDirection = toTarget / distanceToTarget;
                hasWaypointDirection = true;
                moveInput = waypointDirection;
            }
        }
        if (config.lockGoalieDepth) {
            moveInput.y = 0.0f;
        }

        const bool canMove = matchAllowsMovement && playerComponent.activeInMatch && runtime.inputEnabled && runtime.movementEnabled;
        const bool braking = input.brakeHeld || !canMove;
        glm::vec3 targetVelocity = canMove && !braking ? Xz(moveInput) * config.maxSpeed : glm::vec3{0.0f};
        const bool hasMoveInput = LengthSq(moveInput) > 0.0001f;
        const float rate = hasMoveInput && canMove && !braking ? config.acceleration : config.deceleration;

        if (input.brakePressed) {
            if (runtime.lastBrakePressedTime >= 0.0f &&
                runtime.lastBrakePressedTime <= tuning.skater.doubleStopWindowSeconds) {
                runtime.velocity = glm::vec3{0.0f};
                targetVelocity = glm::vec3{0.0f};
            }
            runtime.lastBrakePressedTime = 0.0f;
        }

        bool quickTurned = false;
        if (canMove && input.quickTurnPressed && !player.HasComponent<GoalieComponent>()) {
            const glm::vec3 facing = NormalizedOrForward(runtime.facingDirection);
            runtime.facingDirection = -facing;
            runtime.velocity *= std::clamp(tuning.skater.slideStopDamping, 0.0f, 1.0f);
            runtime.hasMoveTarget = false;
            targetVelocity = glm::vec3{0.0f};
            quickTurned = true;
        }

        runtime.velocity = MoveToward(runtime.velocity, targetVelocity, std::max(0.0f, rate) * fixedDeltaSeconds);

        if (canMove && input.boostPressed) {
            const glm::vec3 direction = NormalizedOrForward(runtime.facingDirection);
            if (player.HasComponent<GoalieComponent>()) {
                if (runtime.goalieBoostCharges > 0) {
                    --runtime.goalieBoostCharges;
                    runtime.velocity += direction * tuning.goalie.boostImpulse;
                    runtime.hasMoveTarget = false;
                    events.Push({GameplayEventType::PlayerBoosted, player.GetUUID(), {}, playerComponent.team, transform.localPosition});
                }
            } else if (runtime.boostCooldown <= 0.0f) {
                runtime.velocity += direction * tuning.skater.boostImpulse;
                runtime.boostCooldown = tuning.skater.boostCooldownSeconds;
                runtime.hasMoveTarget = false;
                events.Push({GameplayEventType::PlayerBoosted, player.GetUUID(), {}, playerComponent.team, transform.localPosition});
            }
        }

        if (canMove && player.HasComponent<GoalieComponent>() && input.goalieShieldPressed && !runtime.shieldActive &&
            runtime.shieldCooldown <= 0.0f) {
            runtime.shieldActive = true;
            runtime.shieldTimer = tuning.goalie.shieldDurationSeconds;
            runtime.shieldCooldown = tuning.goalie.shieldCooldownSeconds;
            events.Push({GameplayEventType::GoalieShieldStarted, player.GetUUID(), {}, playerComponent.team, transform.localPosition});
        }

        ApplyGoalieShield(scene, player, runtime, tuning);

        if (!quickTurned && hasWaypointDirection && !braking) {
            runtime.facingDirection = glm::normalize(Xz(waypointDirection));
        } else if (!quickTurned && hasMoveInput && !braking) {
            runtime.facingDirection = glm::normalize(Xz(moveInput));
        } else if (!quickTurned && glm::length(runtime.velocity) > 0.0001f) {
            runtime.facingDirection = glm::normalize(runtime.velocity);
        }
        ApplyFacingRotation(runtime, transform);

        if (HasDynamicPhysicsBody(player, physicsWorld)) {
            glm::vec3 physicsVelocity = runtime.velocity;
            physicsVelocity.y = 0.0f;
            const glm::vec3 worldPosition = glm::vec3(scene.GetWorldTransform(player)[3]);
            physicsWorld->SetBodyTransform(player, worldPosition, glm::quat(glm::radians(transform.localRotation)));
            physicsWorld->SetLinearVelocity(player, physicsVelocity);
        } else {
            transform.localPosition += runtime.velocity * fixedDeltaSeconds;

            if (physicsWorld != nullptr && physicsWorld->IsInitialized() && physicsWorld->HasBody(player)) {
                const glm::vec3 worldPosition = glm::vec3(scene.GetWorldTransform(player)[3]);
                physicsWorld->SetBodyTransform(player, worldPosition, glm::quat(glm::radians(transform.localRotation)));
                physicsWorld->SetLinearVelocity(player, runtime.velocity);
            }
        }
    }
}

void PlayerMovement::SyncFromPhysics(Scene& scene, PhysicsWorld* physicsWorld) {
    if (physicsWorld == nullptr || !physicsWorld->IsInitialized()) {
        return;
    }

    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent, RigidBodyComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        if (!HasDynamicPhysicsBody(player, physicsWorld)) {
            continue;
        }

        glm::vec3 velocity = physicsWorld->GetLinearVelocity(player);
        velocity.y = 0.0f;

        PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
        runtime.velocity = velocity;
        if (!runtime.hasMoveTarget && glm::length(velocity) > 0.0001f) {
            runtime.facingDirection = glm::normalize(velocity);
        }
        ApplyFacingRotation(runtime, player.GetComponent<TransformComponent>());
    }
}

} // namespace Hockey
