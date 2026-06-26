#include "Hockey/UI/ClientFlowRunner.hpp"

#include <utility>

namespace Hockey {

void ClientFlowRunner::SetFlow(ClientFlow flow) {
    m_Flow = std::move(flow);
}

const ClientFlow& ClientFlowRunner::Flow() const {
    return m_Flow;
}

void ClientFlowRunner::Boot() {
    m_Screens.SetActiveScreen(m_Flow.startupScreen);
    RequestBackgroundFor(m_Flow.startupScreen);
}

UIScreenId ClientFlowRunner::ActiveScreen() const {
    return m_Screens.ActiveScreen();
}

void ClientFlowRunner::Dispatch(UIAction action) {
    switch (action) {
    case UIAction::StartOffline:
        m_Screens.SetActiveScreen(UIScreenId::Loading);
        m_RequestedGameplayScene = m_Flow.offline.defaultScene;
        break;
    case UIAction::OpenLobby:
        m_Screens.SetActiveScreen(UIScreenId::Lobby);
        RequestBackgroundFor(UIScreenId::Lobby);
        break;
    case UIAction::OpenSettings:
        m_Screens.SetActiveScreen(UIScreenId::Settings);
        RequestBackgroundFor(UIScreenId::Settings);
        break;
    case UIAction::Pause:
        m_Screens.SetActiveScreen(UIScreenId::PauseMenu);
        break;
    case UIAction::Resume:
        m_Screens.SetActiveScreen(UIScreenId::MatchHud);
        break;
    case UIAction::ReturnToMenu:
        m_Screens.SetActiveScreen(UIScreenId::Home);
        RequestBackgroundFor(UIScreenId::Home);
        break;
    case UIAction::Quit:
    case UIAction::ReadyToggle:
    case UIAction::SelectTeamRole:
        m_Screens.PushAction(action);
        break;
    }
}

std::optional<std::string> ClientFlowRunner::TakeRequestedGameplayScene() {
    std::optional<std::string> out = std::move(m_RequestedGameplayScene);
    m_RequestedGameplayScene.reset();
    return out;
}

std::optional<std::string> ClientFlowRunner::TakeRequestedBackgroundScene() {
    std::optional<std::string> out = std::move(m_RequestedBackgroundScene);
    m_RequestedBackgroundScene.reset();
    return out;
}

void ClientFlowRunner::RequestBackgroundFor(UIScreenId screen) {
    const std::string& background = m_Flow.BackgroundScene(screen);
    if (!background.empty()) {
        m_RequestedBackgroundScene = background;
    }
}

} // namespace Hockey
