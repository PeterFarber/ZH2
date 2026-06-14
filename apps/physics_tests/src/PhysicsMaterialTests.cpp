#include "Test.hpp"

#include "Hockey/Physics/PhysicsMaterial.hpp"

#include <string>

using namespace Hockey;

void RunPhysicsMaterialTests() {
    HockeyTest::BeginSuite("PhysicsMaterialTests");

    PhysicsMaterialRegistry& registry = PhysicsMaterialRegistry::Get();
    registry.RegisterBuiltIns();

    // Built-ins exist.
    HK_CHECK_MSG(registry.Contains("Default"), "Default material exists");
    HK_CHECK_MSG(registry.Contains("Ice"), "Ice material exists");
    HK_CHECK_MSG(registry.Contains("PuckRubber"), "PuckRubber material exists");
    HK_CHECK_MSG(registry.Contains("Boards"), "Boards material exists");
    HK_CHECK_MSG(registry.Contains("Trigger"), "Trigger material exists");

    const PhysicsMaterial* def = registry.Find("Default");
    const PhysicsMaterial* ice = registry.Find("Ice");
    const PhysicsMaterial* puck = registry.Find("PuckRubber");
    HK_CHECK_MSG(def != nullptr && ice != nullptr && puck != nullptr, "lookups succeed");

    // Ice has low friction relative to default.
    HK_CHECK_MSG(ice != nullptr && def != nullptr && ice->friction < def->friction, "ice friction below default");

    // Puck rubber bounces more than default.
    HK_CHECK_MSG(puck != nullptr && def != nullptr && puck->restitution > def->restitution,
                 "puck restitution above default");

    // Invalid material detected.
    HK_CHECK_MSG(!registry.Contains("NotAMaterial"), "missing material reported absent");
    HK_CHECK_MSG(registry.Find("NotAMaterial") == nullptr, "missing material find returns null");

    // FindOrDefault falls back to Default.
    HK_CHECK_MSG(registry.FindOrDefault("NotAMaterial").name == "Default", "fallback to Default");
    HK_CHECK_MSG(registry.FindOrDefault("Ice").name == "Ice", "find returns requested material");

    // Custom registration / override.
    PhysicsMaterial custom;
    custom.name = "TestSurface";
    custom.friction = 0.123f;
    registry.Register(custom);
    HK_CHECK_MSG(registry.Contains("TestSurface"), "custom material registered");
    HK_CHECK_NEAR(registry.Find("TestSurface")->friction, 0.123f, 1e-5);

    // Re-registering with same name overrides.
    custom.friction = 0.777f;
    registry.Register(custom);
    HK_CHECK_NEAR(registry.Find("TestSurface")->friction, 0.777f, 1e-5);

    // Restore built-ins for downstream tests.
    registry.RegisterBuiltIns();
}
