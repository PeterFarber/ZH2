#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunEntityTests();
void RunComponentTests();
void RunHierarchyTests();
void RunTransformTests();
void RunSceneSerializationTests();
void RunSceneValidationTests();
void RunPrefabTests();
void RunComponentMetadataTests();
void RunSceneLifecycleTests();
void RunSceneEventTests();
void RunRenderComponentTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("ecs_tests.log"));

    RunEntityTests();
    RunComponentTests();
    RunHierarchyTests();
    RunTransformTests();
    RunSceneSerializationTests();
    RunSceneValidationTests();
    RunPrefabTests();
    RunComponentMetadataTests();
    RunSceneLifecycleTests();
    RunSceneEventTests();
    RunRenderComponentTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("ECS tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== ECS tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
