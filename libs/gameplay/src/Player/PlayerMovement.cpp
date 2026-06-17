#include "Hockey/Gameplay/Player/PlayerMovement.hpp"

#include <algorithm>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
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
    float boostMultiplier = 1.0f;
    bool lockGoalieDepth = false;
};

MovementConfig ConfigFor(Entity player, const GameplayTuning& tuning) {
    if (player.HasComponent<GoalieComponent>()) {
        const GoalieComponent& goalie = player.GetComponent<GoalieComponent>();
        return {goalie.maxSpeed,
                goalie.acceleration,
                tuning.skater.deceleration,
                1.0f,
                goalie.lockToCrease};
    }

    const SkaterComponent& skater = player.GetComponent<SkaterComponent>();
    return {skater.maxSpeed, skater.acceleration, skater.deceleration, tuning.skater.sprintMultiplier, false};
}

} // namespace

void PlayerMovement::FixedUpdate(Scene& scene,
                                 PhysicsWorld* physicsWorld,
                                 const GameplayInputBuffer& inputs,
                                 const GameplayTuning& tuning,
                                 float fixedDeltaSeconds) {
    const bool matchAllowsMovement = CanMoveInCurrentMatchPhase(scene);
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();

    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();
        TransformComponent& transform = player.GetComponent<TransformComponent>();

        GameplayInputFrame input;
        inputs.GetInput(playerComponent.playerIndex, input);
        if (input.hasMoveTarget) {
            runtime.moveTarget = input.moveTarget;
            runtime.hasMoveTarget = true;
        }

        const MovementConfig config = ConfigFor(player, tuning);
        glm::vec2 moveInput = ClampedInput(input.move);
        if (runtime.hasMoveTarget) {
            const glm::vec2 toTarget = Xz(runtime.moveTarget - transform.localPosition);
            const float distanceToTarget = glm::length(toTarget);
            if (distanceToTarget <= 0.35f) {
                runtime.hasMoveTarget = false;
                moveInput = {0.0f, 0.0f};
            } else {
                moveInput = toTarget / distanceToTarget;
            }
        }
        if (config.lockGoalieDepth) {
            moveInput.y = 0.0f;
        }

        const bool canMove = matchAllowsMovement && playerComponent.activeInMatch && runtime.inputEnabled && runtime.movementEnabled;
        const bool braking = input.brake || !canMove;
        const bool boosting = input.boostForward || input.sprint;
        const float speedMultiplier = boosting ? config.boostMultiplier : 1.0f;
        glm::vec3 targetVelocity =
            canMove && !braking ? Xz(moveInput) * config.maxSpeed * speedMultiplier : glm::vec3{0.0f};
        const bool hasMoveInput = LengthSq(moveInput) > 0.0001f;
        const float rate = hasMoveInput && canMove && !braking ? config.acceleration : config.deceleration;

        if (input.quickTurnPressed) {
            const glm::vec3 facing = glm::length(runtime.facingDirection) > 0.0001f
                                         ? glm::normalize(runtime.facingDirection)
                                         : glm::vec3{0.0f, 0.0f, 1.0f};
            runtime.facingDirection = -facing;
            runtime.velocity *= -0.35f;
            runtime.hasMoveTarget = false;
            targetVelocity = glm::vec3{0.0f};
        }

        runtime.velocity = MoveToward(runtime.velocity, targetVelocity, std::max(0.0f, rate) * fixedDeltaSeconds);

        if (LengthSq(input.aim) > 0.0001f) {
            runtime.facingDirection = glm::normalize(Xz(ClampedInput(input.aim)));
        } else if (hasMoveInput && !braking) {
            runtime.facingDirection = glm::normalize(Xz(moveInput));
        } else if (glm::length(runtime.velocity) > 0.0001f) {
            runtime.facingDirection = glm::normalize(runtime.velocity);
        }

        transform.localPosition += runtime.velocity * fixedDeltaSeconds;

        if (physicsWorld != nullptr && physicsWorld->IsInitialized() && physicsWorld->HasBody(player)) {
            physicsWorld->SetLinearVelocity(player, runtime.velocity);
        }
    }
}

} // namespace Hockey
