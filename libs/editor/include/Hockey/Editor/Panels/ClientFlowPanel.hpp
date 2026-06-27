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

    ClientFlow m_Flow = MakeDefaultClientFlow();
    std::filesystem::path m_Path;
    std::array<char, 256> m_Home{};
    std::array<char, 256> m_Hud{};
    std::array<char, 256> m_OfflineScene{};
    std::string m_Status;
};

} // namespace Hockey
