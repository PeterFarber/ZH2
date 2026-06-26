#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

// Pure unit tests (no GPU required).
void RunRendererSettingsTests();
void RunShadowQualityTests();
void RunResourceHandleTests();
void RunHeadlessServerLinkTests();
void RunShaderCompileTests();
void RunRenderGraphTests();
void RunRendererShadowContractTests();
void RunMaterialAssetContractTests();
void RunUIOverlayContractTests();

// GPU smoke tests (skipped when no display/GPU is available).
void RunRendererSmokeTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("renderer_tests.log"));

    RunRendererSettingsTests();
    RunShadowQualityTests();
    RunResourceHandleTests();
    RunHeadlessServerLinkTests();
    RunShaderCompileTests();
    RunRenderGraphTests();
    RunRendererShadowContractTests();
    RunMaterialAssetContractTests();
    RunUIOverlayContractTests();

    if (!commandLine.Has("--no-gpu")) {
        RunRendererSmokeTests();
    }

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Renderer tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Renderer tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
