#pragma once

#include <optional>
#include <string>

#include "Hockey/UI/ClientFlow.hpp"
#include "Hockey/UI/UIScreenManager.hpp"

namespace Hockey {

class ClientFlowRunner {
public:
    void SetFlow(ClientFlow flow);
    const ClientFlow& Flow() const;

    void Boot();
    UIScreenId ActiveScreen() const;
    void Dispatch(UIAction action);

    std::optional<std::string> TakeRequestedGameplayScene();
    std::optional<std::string> TakeRequestedBackgroundScene();

private:
    ClientFlow m_Flow = MakeDefaultClientFlow();
    UIScreenManager m_Screens;
    std::optional<std::string> m_RequestedGameplayScene;
    std::optional<std::string> m_RequestedBackgroundScene;

    void RequestBackgroundFor(UIScreenId screen);
};

} // namespace Hockey
