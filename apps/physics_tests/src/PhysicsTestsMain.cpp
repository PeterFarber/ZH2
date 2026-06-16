#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunPhysicsInitTests();
void RunCollisionLayerTests();
void RunPhysicsMaterialTests();
void RunPhysicsComponentTests();
void RunColliderTests();
void RunPhysicsWorldTests();
void RunRigidBodyTests();
void RunSceneSyncTests();
void RunContactEventTests();
void RunTriggerTests();
void RunQueryTests();
void RunDebugDrawTests();
void RunHeadlessServerPhysicsTests();
void RunShapeCastTests();
void RunMaterialCombineModeTests();
void RunSensorLayerTests();
void RunPlayerCapsuleValidationTests();
void RunRaycastAllTests();
void RunTriggerDetectFlagTests();
void RunDeterminismTests();
void RunPhysicsMaterialComponentTests();
void RunMeshColliderProviderTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("physics_tests.log"));

    RunPhysicsInitTests();
    RunCollisionLayerTests();
    RunPhysicsMaterialTests();
    RunPhysicsComponentTests();
    RunColliderTests();
    RunPhysicsWorldTests();
    RunRigidBodyTests();
    RunSceneSyncTests();
    RunContactEventTests();
    RunTriggerTests();
    RunQueryTests();
    RunDebugDrawTests();
    RunHeadlessServerPhysicsTests();
    RunShapeCastTests();
    RunMaterialCombineModeTests();
    RunSensorLayerTests();
    RunPlayerCapsuleValidationTests();
    RunRaycastAllTests();
    RunTriggerDetectFlagTests();
    RunDeterminismTests();
    RunPhysicsMaterialComponentTests();
    RunMeshColliderProviderTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Physics tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Physics tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
