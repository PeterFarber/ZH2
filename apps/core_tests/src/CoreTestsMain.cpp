#include "Test.hpp"
#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunUUIDTests();
void RunConfigTests();
void RunPathTests();
void RunFileSystemTests();
void RunJobSystemTests();
void RunFixedTickTests();
void RunCommandLineTests();
void RunEventQueueTests();
void RunPlatformTests();
void RunTimerTests();
void RunTimeTests();
void RunSignalHandlerTests();
void RunThreadPoolTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("core_tests.log"));

    RunUUIDTests();
    RunConfigTests();
    RunPathTests();
    RunFileSystemTests();
    RunJobSystemTests();
    RunFixedTickTests();
    RunCommandLineTests();
    RunEventQueueTests();
    RunPlatformTests();
    RunTimerTests();
    RunTimeTests();
    RunSignalHandlerTests();
    RunThreadPoolTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Core tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Core tests: %d passed, %d failed ====\n",
                 stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
