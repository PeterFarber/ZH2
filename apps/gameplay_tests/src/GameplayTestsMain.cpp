#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunSettingsTuningTests();
void RunInputModelTests();
void RunEventTests();
void RunTeamTypesTests();
void RunGameplayComponentTests();
void RunMatchSetupTests();
void RunGameplayWorldTests();
void RunResetSystemTests();
void RunSkaterMovementTests();
void RunGoalieMovementTests();
void RunWaypointMarkerTests();
void RunStickHandlingTests();
void RunPuckInteractionTests();
void RunPuckControllerTests();
void RunShootingTests();
void RunStealTests();
void RunGoalDetectionTests();
void RunScoreTests();
void RunOutOfPlayTests();
void RunGameplaySnapshotTests();
void RunMainRinkGameplayTests();
void RunHeadlessServerGameplayTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("gameplay_tests.log"));

    RunSettingsTuningTests();
    RunInputModelTests();
    RunEventTests();
    RunTeamTypesTests();
    RunGameplayComponentTests();
    RunMatchSetupTests();
    RunGameplayWorldTests();
    RunResetSystemTests();
    RunSkaterMovementTests();
    RunGoalieMovementTests();
    RunWaypointMarkerTests();
    RunStickHandlingTests();
    RunPuckInteractionTests();
    RunPuckControllerTests();
    RunShootingTests();
    RunStealTests();
    RunGoalDetectionTests();
    RunScoreTests();
    RunOutOfPlayTests();
    RunGameplaySnapshotTests();
    RunMainRinkGameplayTests();
    RunHeadlessServerGameplayTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Gameplay tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Gameplay tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
