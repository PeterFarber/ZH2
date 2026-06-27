#pragma once

#include <array>
#include <filesystem>

#include "Hockey/Editor/Panel.hpp"
#include "Hockey/UI/ClientFlow.hpp"

namespace Hockey {

class ClientFlowPanel : public Panel {
public:
    ClientFlowPanel();
    void OnImGui(EditorContext& context) override;

private:
    void LoadFromPath(const std::filesystem::path& path);
    void CopyFlowToBuffers();
    void CopyBuffersToFlow();
    void DrawPathField(const char* label, std::array<char, 256>& buffer);
    bool ValidateFlowForAuthoring();

    ClientFlow m_Flow = MakeDefaultClientFlow();
    std::filesystem::path m_Path;
    std::array<char, 256> m_Home{};
    std::array<char, 256> m_Loading{};
    std::array<char, 256> m_Lobby{};
    std::array<char, 256> m_TeamSelect{};
    std::array<char, 256> m_Hud{};
    std::array<char, 256> m_PauseMenu{};
    std::array<char, 256> m_Settings{};
    std::array<char, 256> m_Scoreboard{};
    std::array<char, 256> m_EndMatch{};
    std::array<char, 256> m_HomeBackground{};
    std::array<char, 256> m_LoadingBackground{};
    std::array<char, 256> m_LobbyBackground{};
    std::array<char, 256> m_TeamSelectBackground{};
    std::array<char, 256> m_PauseMenuBackground{};
    std::array<char, 256> m_SettingsBackground{};
    std::array<char, 256> m_ScoreboardBackground{};
    std::array<char, 256> m_EndMatchBackground{};
    std::array<char, 256> m_OfflineScene{};
    bool m_UseCurrentEditorSceneWhenPreviewing = true;
    std::string m_Status;
};

} // namespace Hockey
