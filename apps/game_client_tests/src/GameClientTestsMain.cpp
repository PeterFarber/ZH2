#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunGameplayCameraTests();
void RunGameplayPresentationTests();
void RunConfigPathTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("game_client_tests.log"));

    RunGameplayCameraTests();
    RunGameplayPresentationTests();
    RunConfigPathTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Game client tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Game client tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
