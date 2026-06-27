#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunUISettingsTests();
void RunClientFlowTests();
void RunUIAssetContractTests();
void RunUIArchitectureContractTests();
void RunRmlUiInterfaceTests();
void RunRmlUiContextTests();
void RunUIInputMapperTests();
void RunHudViewModelTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("ui_tests.log"));

    RunUISettingsTests();
    RunClientFlowTests();
    RunUIAssetContractTests();
    RunUIArchitectureContractTests();
    RunRmlUiInterfaceTests();
    RunRmlUiContextTests();
    RunUIInputMapperTests();
    RunHudViewModelTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    std::fprintf(stderr, "\n==== UI tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
