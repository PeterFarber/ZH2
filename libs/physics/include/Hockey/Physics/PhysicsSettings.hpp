#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

class Config;

// ---------------------------------------------------------------------------
// Tunable parameters for a physics world. Defaults are chosen to be stable for
// a 4v4 hockey simulation; apps override them from their *.toml config files.
// ---------------------------------------------------------------------------
struct PhysicsSettings {
    float fixedDeltaSeconds = 1.0f / 60.0f;

    std::uint32_t maxBodies = 65536;
    std::uint32_t maxBodyPairs = 65536;
    std::uint32_t maxContactConstraints = 20000;

    std::uint32_t tempAllocatorSizeMB = 64;
    std::uint32_t jobThreadCount = 0; // 0 == auto (hardware_concurrency - 1)

    glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    std::uint32_t collisionSteps = 1;
    std::uint32_t integrationSubsteps = 1;

    bool deterministicMode = false;
    bool enableSleeping = true;
    bool enableDebugDraw = false;

    float worldMinY = -1000.0f;
};

// Returns the engine defaults (same as a value-initialised PhysicsSettings).
PhysicsSettings MakeDefaultPhysicsSettings();

// Reads the [physics] table from a Config into outSettings. Missing keys keep
// their current/default value. Gravity is stored as the scalar keys
// physics.gravity_x / physics.gravity_y / physics.gravity_z so it round-trips
// through the scalar-only Config API. Always succeeds.
Status LoadPhysicsSettings(const Config& config, PhysicsSettings& outSettings);
void SavePhysicsSettings(Config& config, const PhysicsSettings& settings);

} // namespace Hockey
