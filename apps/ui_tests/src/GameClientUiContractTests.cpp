#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

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

void RunGameClientUiContractTests() {
    HockeyTest::BeginSuite("GameClientUiContracts");

    const std::string source = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    const std::string mainSource = ReadProjectFile("apps/game_client/src/GameClientMain.cpp");
    const std::string header = ReadProjectFile("apps/game_client/src/GameClientApp.hpp");
    const std::string cmake = ReadProjectFile("apps/game_client/CMakeLists.txt");

    HK_CHECK_MSG(Contains(cmake, "hockey_ui"), "HockeyGameClient links hockey_ui");
    HK_CHECK_MSG(!Contains(cmake, "hockey_editor"), "HockeyGameClient does not link hockey_editor");
    HK_CHECK_MSG(Contains(source, "--no-ui"), "HockeyGameClient parses --no-ui");
    HK_CHECK_MSG(Contains(mainSource, "--no-ui") && Contains(mainSource, "Disable runtime RmlUi"),
                 "HockeyGameClient help documents --no-ui");
    HK_CHECK_MSG(Contains(header, "LoadOfflineGameplayScene"),
                 "GameClientApp has an explicit offline scene load path for client-flow requests");
    HK_CHECK_MSG(!Contains(source, "(void)m_ClientFlow.TakeRequestedGameplayScene();"),
                 "GameClientApp must not discard client-flow gameplay scene requests");
    HK_CHECK_MSG(Contains(source, "m_ClientFlow.ActiveScreen() != Hockey::UIScreenId::MatchHud"),
                 "GameClientApp gates gameplay input while non-HUD UI screens are active");
    HK_CHECK_MSG(!Contains(source,
                           "m_ClientFlow.ActiveScreen() != Hockey::UIScreenId::MatchHud || "
                           "m_UIInput.WantsMouseCapture()") &&
                     !Contains(source,
                               "m_ClientFlow.ActiveScreen() != Hockey::UIScreenId::MatchHud || "
                               "m_UIInput.WantsKeyboardCapture()"),
                 "GameClientApp keeps HUD gameplay input live even when RmlUi reports capture");
}
