#pragma once

#include <optional>
#include <vector>

#include "Hockey/UI/ClientFlow.hpp"

namespace Hockey {

enum class UIAction {
    StartOffline,
    OpenLobby,
    OpenSettings,
    Quit,
    Pause,
    Resume,
    ReturnToMenu,
    ReadyToggle,
    SelectTeamRole,
};

class UIScreenManager {
public:
    void SetActiveScreen(UIScreenId screen);
    UIScreenId ActiveScreen() const;

    void ShowHud(bool show);
    bool HudVisible() const;
    bool GameplayInputAllowed() const;

    void PushAction(UIAction action);
    std::optional<UIAction> PopAction();

private:
    UIScreenId m_ActiveScreen = UIScreenId::Home;
    bool m_HudVisible = false;
    std::vector<UIAction> m_Actions;
};

} // namespace Hockey
