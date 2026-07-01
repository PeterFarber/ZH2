#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunAudioSettingsTests();
void RunAudioMixerTests();
void RunAudioClipTests();
void RunAudioSystemTests();
void RunAudioComponentTests();
void RunHockeyAudioControllerTests();
void RunHockeyAudioCueSettingsTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("audio_tests.log"));

    RunAudioSettingsTests();
    RunAudioMixerTests();
    RunAudioClipTests();
    RunAudioSystemTests();
    RunAudioComponentTests();
    RunHockeyAudioControllerTests();
    RunHockeyAudioCueSettingsTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    std::fprintf(stderr, "\n==== Audio tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}
