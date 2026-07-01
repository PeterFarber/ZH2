#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunAnimationLibraryContractTests();
void RunAnimationSettingsTests();
void RunSkeletonTests();
void RunAnimationClipTests();
void RunAnimationSamplerTests();
void RunAnimationBlendTests();
void RunAnimationComponentSerializerTests();
void RunAnimationGraphTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("animation_tests.log"));

    RunAnimationLibraryContractTests();
    RunAnimationSettingsTests();
    RunSkeletonTests();
    RunAnimationClipTests();
    RunAnimationSamplerTests();
    RunAnimationBlendTests();
    RunAnimationComponentSerializerTests();
    RunAnimationGraphTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    std::fprintf(stderr, "\n==== Animation tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
