#include "Test.hpp"

#include <filesystem>
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

bool Exists(const char* relativePath) {
    return std::filesystem::exists(Hockey::Paths::Get().root / relativePath);
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunUIAssetContractTests() {
    HockeyTest::BeginSuite("UIAssetContracts");

    HK_CHECK(Exists("data/ui/README.md"));
    HK_CHECK(Exists("data/ui/fonts/README.md"));
    HK_CHECK(Exists("data/ui/fonts/Inter-Regular.ttf"));
    HK_CHECK(Exists("data/ui/fonts/Inter-Regular.ttf.license.txt"));
    HK_CHECK(Exists("data/ui/themes/hockey.rcss"));
    HK_CHECK(Exists("data/ui/flows/default.clientflow.yaml"));

    const char* screens[] = {
        "home",       "loading",    "lobby",      "team_select", "match_hud",
        "pause_menu", "settings",   "scoreboard", "end_match",
    };
    for (const char* screen : screens) {
        const std::string path = std::string("data/ui/screens/") + screen + ".rml";
        HK_CHECK_MSG(Exists(path.c_str()), path + " exists");
    }

    const std::string home = ReadProjectFile("data/ui/screens/home.rml");
    HK_CHECK_MSG(Contains(home, "id=\"play-offline\""), "home screen has Play Offline button id");
    HK_CHECK_MSG(Contains(home, "id=\"open-lobby\""), "home screen has Lobby button id");
    HK_CHECK_MSG(Contains(home, "id=\"open-settings\""), "home screen has Settings button id");
    HK_CHECK_MSG(Contains(home, "id=\"quit-game\""), "home screen has Quit button id");

    const std::string hud = ReadProjectFile("data/ui/screens/match_hud.rml");
    HK_CHECK_MSG(Contains(hud, "id=\"home-score\""), "HUD has home score id");
    HK_CHECK_MSG(Contains(hud, "id=\"away-score\""), "HUD has away score id");
    HK_CHECK_MSG(Contains(hud, "id=\"match-clock\""), "HUD has match clock id");
    HK_CHECK_MSG(Contains(hud, "id=\"shot-charge\""), "HUD has shot charge id");
}
