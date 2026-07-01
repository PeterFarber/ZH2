#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunGameClientAudioContractTests() {
    HockeyTest::BeginSuite("GameClientAudioContractTests");

    const std::string cmake = ReadProjectFile("apps/game_client/CMakeLists.txt");
    const std::string header = ReadProjectFile("apps/game_client/src/GameClientApp.hpp");
    const std::string source = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    const std::string defaults = ReadProjectFile("libs/core/src/RuntimeConfigDefaults.cpp");
    const std::string rootCmake = ReadProjectFile("CMakeLists.txt");

    HK_CHECK_MSG(Contains(cmake, "hockey_audio"), "game client links hockey_audio");
    HK_CHECK_MSG(Contains(header, "AudioSystem"), "game client owns audio system");
    HK_CHECK_MSG(Contains(header, "HockeyAudioController"), "game client owns hockey audio controller");
    HK_CHECK_MSG(Contains(source, "RegisterAudioComponents"),
                 "game client registers audio components before scene load");
    HK_CHECK_MSG(Contains(source, "LoadAudioSettings"), "game client loads audio settings");
    HK_CHECK_MSG(Contains(source, "HandleAudioGameplayEvent"), "game client routes gameplay events to audio");
    HK_CHECK_MSG(Contains(source, "HandleAudioPhysicsContact"), "game client routes physics contacts to audio");
    HK_CHECK_MSG(Contains(source, "QueueRuntimeUIAction") && Contains(source, "AudioBusType::UI"),
                 "game client routes runtime UI actions to UI audio cues");
    HK_CHECK_MSG(Contains(defaults, "[audio]"), "runtime defaults contain audio section");
    HK_CHECK_MSG(Contains(rootCmake, "HockeyDedicatedServer must not link hockey_audio"),
                 "root CMake guards dedicated server from audio");
}
