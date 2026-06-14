#include "Hockey/Physics/PhysicsSettings.hpp"

#include "Hockey/Core/Config.hpp"

namespace Hockey {

PhysicsSettings MakeDefaultPhysicsSettings() {
    return PhysicsSettings{};
}

namespace {

std::uint32_t LoadU32(const Config& config, std::string_view key, std::uint32_t fallback) {
    return static_cast<std::uint32_t>(config.GetInt(key, static_cast<int>(fallback)));
}

float LoadF(const Config& config, std::string_view key, float fallback) {
    return static_cast<float>(config.GetDouble(key, static_cast<double>(fallback)));
}

} // namespace

Status LoadPhysicsSettings(const Config& config, PhysicsSettings& s) {
    s.fixedDeltaSeconds = LoadF(config, "physics.fixed_delta_seconds", s.fixedDeltaSeconds);

    s.maxBodies = LoadU32(config, "physics.max_bodies", s.maxBodies);
    s.maxBodyPairs = LoadU32(config, "physics.max_body_pairs", s.maxBodyPairs);
    s.maxContactConstraints = LoadU32(config, "physics.max_contact_constraints", s.maxContactConstraints);

    s.tempAllocatorSizeMB = LoadU32(config, "physics.temp_allocator_size_mb", s.tempAllocatorSizeMB);
    s.jobThreadCount = LoadU32(config, "physics.job_thread_count", s.jobThreadCount);

    s.gravity.x = LoadF(config, "physics.gravity_x", s.gravity.x);
    s.gravity.y = LoadF(config, "physics.gravity_y", s.gravity.y);
    s.gravity.z = LoadF(config, "physics.gravity_z", s.gravity.z);

    s.collisionSteps = LoadU32(config, "physics.collision_steps", s.collisionSteps);
    s.integrationSubsteps = LoadU32(config, "physics.integration_substeps", s.integrationSubsteps);

    s.deterministicMode = config.GetBool("physics.deterministic_mode", s.deterministicMode);
    s.enableSleeping = config.GetBool("physics.enable_sleeping", s.enableSleeping);
    s.enableDebugDraw = config.GetBool("physics.enable_debug_draw", s.enableDebugDraw);

    s.worldMinY = LoadF(config, "physics.world_min_y", s.worldMinY);

    return Status::Ok();
}

void SavePhysicsSettings(Config& config, const PhysicsSettings& s) {
    config.SetDouble("physics.fixed_delta_seconds", s.fixedDeltaSeconds);

    config.SetInt("physics.max_bodies", static_cast<int>(s.maxBodies));
    config.SetInt("physics.max_body_pairs", static_cast<int>(s.maxBodyPairs));
    config.SetInt("physics.max_contact_constraints", static_cast<int>(s.maxContactConstraints));

    config.SetInt("physics.temp_allocator_size_mb", static_cast<int>(s.tempAllocatorSizeMB));
    config.SetInt("physics.job_thread_count", static_cast<int>(s.jobThreadCount));

    config.SetDouble("physics.gravity_x", s.gravity.x);
    config.SetDouble("physics.gravity_y", s.gravity.y);
    config.SetDouble("physics.gravity_z", s.gravity.z);

    config.SetInt("physics.collision_steps", static_cast<int>(s.collisionSteps));
    config.SetInt("physics.integration_substeps", static_cast<int>(s.integrationSubsteps));

    config.SetBool("physics.deterministic_mode", s.deterministicMode);
    config.SetBool("physics.enable_sleeping", s.enableSleeping);
    config.SetBool("physics.enable_debug_draw", s.enableDebugDraw);

    config.SetDouble("physics.world_min_y", s.worldMinY);
}

} // namespace Hockey
