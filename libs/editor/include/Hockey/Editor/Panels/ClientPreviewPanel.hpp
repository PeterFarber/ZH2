#pragma once

#include <cstdint>

#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

class ClientPreviewPanel : public Panel {
public:
    ClientPreviewPanel();
    void OnImGui(EditorContext& context) override;

private:
    void EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);

    TextureHandle m_Target{};
    std::uint32_t m_Width = 0;
    std::uint32_t m_Height = 0;
};

} // namespace Hockey
