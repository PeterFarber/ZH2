#include "Hockey/UI/ClientFlow.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

namespace Hockey {

namespace {

using ScreenName = std::pair<UIScreenId, const char*>;

constexpr std::array<ScreenName, 9> kScreenNames{{
    {UIScreenId::Home, "home"},
    {UIScreenId::Loading, "loading"},
    {UIScreenId::Lobby, "lobby"},
    {UIScreenId::TeamSelect, "team_select"},
    {UIScreenId::MatchHud, "match_hud"},
    {UIScreenId::PauseMenu, "pause_menu"},
    {UIScreenId::Settings, "settings"},
    {UIScreenId::Scoreboard, "scoreboard"},
    {UIScreenId::EndMatch, "end_match"},
}};

bool IEquals(const std::string& a, const char* b) {
    const std::string_view rhs{b};
    if (a.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

const char* ToString(UIScreenId id) {
    for (const auto& [value, name] : kScreenNames) {
        if (value == id) {
            return name;
        }
    }
    return "home";
}

bool FromString(const std::string& text, UIScreenId& out) {
    for (const auto& [value, name] : kScreenNames) {
        if (IEquals(text, name)) {
            out = value;
            return true;
        }
    }
    return false;
}

const std::string& ClientFlow::ScreenDocument(UIScreenId id) const {
    switch (id) {
    case UIScreenId::Home:
        return screens.home;
    case UIScreenId::Loading:
        return screens.loading;
    case UIScreenId::Lobby:
        return screens.lobby;
    case UIScreenId::TeamSelect:
        return screens.teamSelect;
    case UIScreenId::MatchHud:
        return screens.matchHud;
    case UIScreenId::PauseMenu:
        return screens.pauseMenu;
    case UIScreenId::Settings:
        return screens.settings;
    case UIScreenId::Scoreboard:
        return screens.scoreboard;
    case UIScreenId::EndMatch:
        return screens.endMatch;
    }
    return screens.home;
}

std::string& ClientFlow::ScreenDocument(UIScreenId id) {
    return const_cast<std::string&>(static_cast<const ClientFlow&>(*this).ScreenDocument(id));
}

const std::string& ClientFlow::BackgroundScene(UIScreenId id) const {
    switch (id) {
    case UIScreenId::Home:
        return backgrounds.home;
    case UIScreenId::Loading:
        return backgrounds.loading;
    case UIScreenId::Lobby:
        return backgrounds.lobby;
    case UIScreenId::TeamSelect:
        return backgrounds.teamSelect;
    case UIScreenId::PauseMenu:
        return backgrounds.pauseMenu;
    case UIScreenId::Settings:
        return backgrounds.settings;
    case UIScreenId::Scoreboard:
        return backgrounds.scoreboard;
    case UIScreenId::EndMatch:
        return backgrounds.endMatch;
    case UIScreenId::MatchHud:
        break;
    }
    static const std::string empty;
    return empty;
}

std::string& ClientFlow::BackgroundScene(UIScreenId id) {
    switch (id) {
    case UIScreenId::Home:
        return backgrounds.home;
    case UIScreenId::Loading:
        return backgrounds.loading;
    case UIScreenId::Lobby:
        return backgrounds.lobby;
    case UIScreenId::TeamSelect:
        return backgrounds.teamSelect;
    case UIScreenId::PauseMenu:
        return backgrounds.pauseMenu;
    case UIScreenId::Settings:
        return backgrounds.settings;
    case UIScreenId::Scoreboard:
        return backgrounds.scoreboard;
    case UIScreenId::EndMatch:
        return backgrounds.endMatch;
    case UIScreenId::MatchHud:
        break;
    }
    return backgrounds.home;
}

ClientFlow MakeDefaultClientFlow() {
    ClientFlow flow;
    flow.startupScreen = UIScreenId::Home;
    flow.screens.home = "data/ui/screens/home.rml";
    flow.screens.loading = "data/ui/screens/loading.rml";
    flow.screens.lobby = "data/ui/screens/lobby.rml";
    flow.screens.teamSelect = "data/ui/screens/team_select.rml";
    flow.screens.matchHud = "data/ui/screens/match_hud.rml";
    flow.screens.pauseMenu = "data/ui/screens/pause_menu.rml";
    flow.screens.settings = "data/ui/screens/settings.rml";
    flow.screens.scoreboard = "data/ui/screens/scoreboard.rml";
    flow.screens.endMatch = "data/ui/screens/end_match.rml";
    flow.offline.defaultScene = "data/raw/scenes/Main.scene.yaml";
    flow.offline.useCurrentEditorSceneWhenPreviewing = true;
    return flow;
}

std::vector<std::string> ValidateClientFlow(const ClientFlow& flow) {
    std::vector<std::string> errors;
    for (const auto& [id, name] : kScreenNames) {
        if (flow.ScreenDocument(id).empty()) {
            errors.emplace_back(std::string("missing required screen document: screens.") + name);
        }
    }
    if (flow.offline.defaultScene.empty()) {
        errors.emplace_back("missing offline.default_scene");
    }
    return errors;
}

} // namespace Hockey
