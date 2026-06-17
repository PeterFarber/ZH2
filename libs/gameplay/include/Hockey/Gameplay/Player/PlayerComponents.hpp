#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

struct PlayerComponent {
    uint32_t playerIndex = 0;
    PlayerSlot slot = PlayerSlot::None;
    GameplayTeam team = GameplayTeam::None;
    GameplayRole role = GameplayRole::Skater;
    bool controlledByLocalInput = false;
    bool controlledByAI = false;
    bool activeInMatch = true;
};

struct SkaterComponent {
    float maxSpeed = 9.0f;
    float acceleration = 32.0f;
    float deceleration = 20.0f;
    float turnSpeedDegrees = 720.0f;
    bool hasPuck = false;
};

struct GoalieComponent {
    float maxSpeed = 6.5f;
    float acceleration = 28.0f;
    float saveReachRadius = 1.8f;
    bool lockToCrease = true;
};

struct PlayerRuntimeComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 facingDirection{0.0f, 0.0f, 1.0f};
    glm::vec3 moveTarget{0.0f};
    float sprintEnergy = 1.0f;
    float checkCooldown = 0.0f;
    float pokeCheckCooldown = 0.0f;
    bool hasMoveTarget = false;
    bool inputEnabled = true;
    bool movementEnabled = true;
};

struct RespawnComponent {
    bool pending = false;
    float timer = 0.0f;
    glm::vec3 targetPosition{0.0f};
};

template <> struct ComponentTraits<PlayerComponent> {
    static constexpr const char* Name = "PlayerComponent";
};
template <> struct ComponentTraits<SkaterComponent> {
    static constexpr const char* Name = "SkaterComponent";
};
template <> struct ComponentTraits<GoalieComponent> {
    static constexpr const char* Name = "GoalieComponent";
};
template <> struct ComponentTraits<PlayerRuntimeComponent> {
    static constexpr const char* Name = "PlayerRuntimeComponent";
};
template <> struct ComponentTraits<RespawnComponent> {
    static constexpr const char* Name = "RespawnComponent";
};

}
