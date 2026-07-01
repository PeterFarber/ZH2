#pragma once

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

namespace Hockey {

struct SkaterTuning {
    float maxSpeed = 9.0f;
    float acceleration = 32.0f;
    float deceleration = 20.0f;
    float turnSpeedDegrees = 720.0f;
    float boostImpulse = 7.5f;
    float boostCooldownSeconds = 1.25f;
    float slideStopDamping = 0.35f;
    float doubleStopWindowSeconds = 0.30f;
    float puckControlRadius = 1.25f;
    float stealRadius = 1.5f;
    float stealCooldownSeconds = 0.35f;
};

struct GoalieTuning {
    float maxSpeed = 6.5f;
    float acceleration = 28.0f;
    float creaseMoveMultiplier = 1.0f;
    float saveReachRadius = 1.8f;
    uint32_t boostCharges = 2;
    float boostRechargeSeconds = 4.0f;
    float boostImpulse = 8.0f;
    float shieldRadius = 2.0f;
    float shieldDurationSeconds = 1.0f;
    float shieldCooldownSeconds = 5.0f;
    float shieldReflectImpulse = 22.0f;
};

struct PuckTuning {
    float maxSpeed = 45.0f;
    glm::vec3 possessionOffset{0.0f, 0.0f, 1.1f};
    float loosePuckDrag = 0.15f;
    float floorY = 0.05f;
    float outOfPlayY = -5.0f;
};

struct StickTuning {
    std::filesystem::path prefabPath = "data/raw/prefabs/Skater_Stick.prefab.yaml";
    float reach = 1.5f;
    float width = 0.25f;
    glm::vec3 localOffset{0.0f, 0.0f, 1.3f};
};

struct ShotTuning {
    float minPower = 8.0f;
    float maxPower = 32.0f;
    float chargeSeconds = 1.0f;
    float selfCollisionGraceSeconds = 0.20f;
    float accuracyDegrees = 4.0f;
};

struct GameplayTuning {
    SkaterTuning skater;
    GoalieTuning goalie;
    PuckTuning puck;
    StickTuning skaterStick;
    StickTuning goalieStick{"data/raw/prefabs/Goalie_Stick.prefab.yaml", 2.0f, 0.35f, {0.0f, 0.0f, 1.55f}};
    ShotTuning shot;
};

}
