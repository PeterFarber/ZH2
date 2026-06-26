#pragma once

#include <string>
#include <vector>

namespace Hockey {

enum class UIScreenId {
    Home,
    Loading,
    Lobby,
    TeamSelect,
    MatchHud,
    PauseMenu,
    Settings,
    Scoreboard,
    EndMatch,
};

const char* ToString(UIScreenId id);
bool FromString(const std::string& text, UIScreenId& out);

struct ClientFlowScreens {
    std::string home;
    std::string loading;
    std::string lobby;
    std::string teamSelect;
    std::string matchHud;
    std::string pauseMenu;
    std::string settings;
    std::string scoreboard;
    std::string endMatch;
};

struct ClientFlowBackgrounds {
    std::string home;
    std::string loading;
    std::string lobby;
    std::string teamSelect;
    std::string pauseMenu;
    std::string settings;
    std::string scoreboard;
    std::string endMatch;
};

struct ClientFlowOffline {
    std::string defaultScene;
    bool useCurrentEditorSceneWhenPreviewing = true;
};

struct ClientFlow {
    UIScreenId startupScreen = UIScreenId::Home;
    ClientFlowScreens screens;
    ClientFlowBackgrounds backgrounds;
    ClientFlowOffline offline;

    const std::string& ScreenDocument(UIScreenId id) const;
    std::string& ScreenDocument(UIScreenId id);
    const std::string& BackgroundScene(UIScreenId id) const;
    std::string& BackgroundScene(UIScreenId id);
};

ClientFlow MakeDefaultClientFlow();
std::vector<std::string> ValidateClientFlow(const ClientFlow& flow);

} // namespace Hockey
