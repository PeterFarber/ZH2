#include "Hockey/UI/UIScreenManager.hpp"

namespace Hockey {

void UIScreenManager::SetActiveScreen(UIScreenId screen) {
    m_ActiveScreen = screen;
    m_HudVisible = screen == UIScreenId::MatchHud;
}

UIScreenId UIScreenManager::ActiveScreen() const {
    return m_ActiveScreen;
}

void UIScreenManager::ShowHud(bool show) {
    m_HudVisible = show;
}

bool UIScreenManager::HudVisible() const {
    return m_HudVisible;
}

bool UIScreenManager::GameplayInputAllowed() const {
    return m_ActiveScreen == UIScreenId::MatchHud;
}

void UIScreenManager::PushAction(UIAction action) {
    m_Actions.push_back(action);
}

std::optional<UIAction> UIScreenManager::PopAction() {
    if (m_Actions.empty()) {
        return std::nullopt;
    }
    const UIAction action = m_Actions.front();
    m_Actions.erase(m_Actions.begin());
    return action;
}

} // namespace Hockey
