#include "Test.hpp"

#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"

#include "Hockey/Core/Config.hpp"

using namespace Hockey;

void RunPhysicsInitTests() {
    HockeyTest::BeginSuite("PhysicsInitTests");

    HK_CHECK_MSG(!Physics::IsInitialized(), "physics not initialized before Init");

    HK_CHECK_MSG(static_cast<bool>(Physics::Init()), "Physics::Init succeeds");
    HK_CHECK_MSG(Physics::IsInitialized(), "physics initialized after Init");

    // Double init must be safe and idempotent.
    HK_CHECK_MSG(static_cast<bool>(Physics::Init()), "Physics::Init double-call succeeds");
    HK_CHECK_MSG(Physics::IsInitialized(), "physics still initialized after double Init");

    Physics::Shutdown();
    HK_CHECK_MSG(!Physics::IsInitialized(), "physics not initialized after Shutdown");

    // Double shutdown must be safe.
    Physics::Shutdown();
    HK_CHECK_MSG(!Physics::IsInitialized(), "double Shutdown safe");

    // Re-init after shutdown must work (whole-process cycle).
    HK_CHECK_MSG(static_cast<bool>(Physics::Init()), "Physics re-Init after shutdown");
    Physics::Shutdown();

    // Default settings sanity.
    const PhysicsSettings defaults = MakeDefaultPhysicsSettings();
    HK_CHECK_NEAR(defaults.fixedDeltaSeconds, 1.0f / 60.0f, 1e-6);
    HK_CHECK_NEAR(defaults.gravity.y, -9.81f, 1e-4);
    HK_CHECK_MSG(defaults.maxBodies > 0, "default maxBodies positive");

    // Settings round-trip through a Config.
    Config config;
    PhysicsSettings custom = MakeDefaultPhysicsSettings();
    custom.gravity = glm::vec3(0.0f, -20.0f, 0.0f);
    custom.fixedDeltaSeconds = 1.0f / 120.0f;
    custom.deterministicMode = true;
    SavePhysicsSettings(config, custom);

    PhysicsSettings loaded = MakeDefaultPhysicsSettings();
    HK_CHECK_MSG(static_cast<bool>(LoadPhysicsSettings(config, loaded)), "LoadPhysicsSettings ok");
    HK_CHECK_NEAR(loaded.gravity.y, -20.0f, 1e-4);
    HK_CHECK_NEAR(loaded.fixedDeltaSeconds, 1.0f / 120.0f, 1e-6);
    HK_CHECK_MSG(loaded.deterministicMode, "deterministic mode round-trips");
}
