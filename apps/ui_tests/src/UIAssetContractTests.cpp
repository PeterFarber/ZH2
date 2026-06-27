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

    const std::string theme = ReadProjectFile("data/ui/themes/hockey.rcss");
    HK_CHECK_MSG(Contains(theme, "button:hover"), "theme defines button hover state");
    HK_CHECK_MSG(Contains(theme, "button:active"), "theme defines button active/click state");
    HK_CHECK_MSG(Contains(theme, "button:focus"), "theme defines button focus state");
    HK_CHECK_MSG(Contains(theme, "button:disabled"), "theme defines button disabled state");
    HK_CHECK_MSG(Contains(theme, ".menu-panel"), "theme defines menu panel layout");
    HK_CHECK_MSG(Contains(theme, ".button-stack"), "theme defines button stack layout");
    HK_CHECK_MSG(Contains(theme, ".hud-pill"), "theme defines HUD pill layout");
    HK_CHECK_MSG(Contains(theme, ".shot-meter"), "theme defines shot meter layout");

    const std::string home = ReadProjectFile("data/ui/screens/home.rml");
    HK_CHECK_MSG(Contains(home, "id=\"play-offline\""), "home screen has Play Offline button id");
    HK_CHECK_MSG(Contains(home, "id=\"open-lobby\""), "home screen has Lobby button id");
    HK_CHECK_MSG(Contains(home, "id=\"open-settings\""), "home screen has Settings button id");
    HK_CHECK_MSG(Contains(home, "id=\"quit-game\""), "home screen has Quit button id");
    HK_CHECK_MSG(Contains(home, "class=\"menu-panel home-panel\""), "home screen uses polished menu panel");
    HK_CHECK_MSG(Contains(home, "class=\"button-stack\""), "home screen groups buttons in a polished stack");

    const std::string hud = ReadProjectFile("data/ui/screens/match_hud.rml");
    HK_CHECK_MSG(Contains(hud, "id=\"home-score\""), "HUD has home score id");
    HK_CHECK_MSG(Contains(hud, "id=\"away-score\""), "HUD has away score id");
    HK_CHECK_MSG(Contains(hud, "id=\"match-clock\""), "HUD has match clock id");
    HK_CHECK_MSG(Contains(hud, "id=\"shot-charge\""), "HUD has shot charge id");
    HK_CHECK_MSG(Contains(hud, "id=\"period-label\""), "HUD has period label id");
    HK_CHECK_MSG(Contains(hud, "id=\"phase-label\""), "HUD has phase label id");
    HK_CHECK_MSG(Contains(hud, "id=\"local-player-label\""), "HUD has local player label id");
    HK_CHECK_MSG(Contains(hud, "id=\"possession-label\""), "HUD has possession label id");
    HK_CHECK_MSG(Contains(hud, "id=\"hud-notification\""), "HUD has notification id");
    HK_CHECK_MSG(Contains(hud, "class=\"score-team home-team\""), "HUD styles home score team block");
    HK_CHECK_MSG(Contains(hud, "class=\"score-team away-team\""), "HUD styles away score team block");
    HK_CHECK_MSG(Contains(hud, "class=\"shot-meter\""), "HUD styles shot charge as a meter");

    const std::string loading = ReadProjectFile("data/ui/screens/loading.rml");
    HK_CHECK_MSG(Contains(loading, "class=\"menu-panel loading-panel\""), "loading screen uses polished menu panel");

    const std::string lobby = ReadProjectFile("data/ui/screens/lobby.rml");
    HK_CHECK_MSG(Contains(lobby, "id=\"lobby-back\""), "lobby screen keeps Back button id");
    HK_CHECK_MSG(Contains(lobby, "class=\"menu-panel lobby-panel\""), "lobby screen uses polished menu panel");

    const std::string teamSelect = ReadProjectFile("data/ui/screens/team_select.rml");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-home-skater\""), "team select keeps Home Skater button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-away-skater\""), "team select keeps Away Skater button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"select-goalie\""), "team select keeps Goalie button id");
    HK_CHECK_MSG(Contains(teamSelect, "id=\"team-ready\""), "team select keeps Ready button id");
    HK_CHECK_MSG(Contains(teamSelect, "class=\"menu-panel team-panel\""), "team select uses polished menu panel");

    const std::string pause = ReadProjectFile("data/ui/screens/pause_menu.rml");
    HK_CHECK_MSG(Contains(pause, "id=\"resume-game\""), "pause menu keeps Resume button id");
    HK_CHECK_MSG(Contains(pause, "id=\"return-to-menu\""), "pause menu keeps Return to Menu button id");
    HK_CHECK_MSG(Contains(pause, "id=\"pause-quit\""), "pause menu keeps Quit button id");
    HK_CHECK_MSG(Contains(pause, "class=\"menu-panel pause-panel\""), "pause menu uses polished menu panel");

    const std::string settings = ReadProjectFile("data/ui/screens/settings.rml");
    HK_CHECK_MSG(Contains(settings, "id=\"ui-scale\""), "settings screen keeps UI scale input id");
    HK_CHECK_MSG(Contains(settings, "id=\"settings-back\""), "settings screen keeps Back button id");
    HK_CHECK_MSG(Contains(settings, "class=\"menu-panel settings-panel\""), "settings screen uses polished menu panel");

    const std::string scoreboard = ReadProjectFile("data/ui/screens/scoreboard.rml");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-home\""), "scoreboard keeps home score id");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-away\""), "scoreboard keeps away score id");
    HK_CHECK_MSG(Contains(scoreboard, "id=\"scoreboard-close\""), "scoreboard keeps Close button id");
    HK_CHECK_MSG(Contains(scoreboard, "class=\"menu-panel scoreboard-panel\""), "scoreboard uses polished menu panel");

    const std::string endMatch = ReadProjectFile("data/ui/screens/end_match.rml");
    HK_CHECK_MSG(Contains(endMatch, "id=\"final-score\""), "end match keeps final score id");
    HK_CHECK_MSG(Contains(endMatch, "id=\"end-return-to-menu\""), "end match keeps Return to Menu button id");
    HK_CHECK_MSG(Contains(endMatch, "class=\"menu-panel final-panel\""), "end match uses polished menu panel");
}
