#pragma once

#include <cstdint>

#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

// Game camera preview. Renders the active scene through its primary
// CameraComponent into an offscreen target and displays it. Shows a hint when
// no usable camera exists, and offers a "Set as Primary" overlay action for a
// selected camera. During play mode this panel owns gameplay input when focused
// or hovered.
class GameViewportPanel : public Panel {
public:
    GameViewportPanel();

    void OnImGui(EditorContext& context) override;

private:
    void DrawViewport(EditorContext& context);
    void EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);
    void EnsureCaptureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);

    TextureHandle m_Target{};
    std::uint32_t m_Width = 0;
    std::uint32_t m_Height = 0;
    TextureHandle m_CaptureTarget{};
    std::uint32_t m_CaptureWidth = 0;
    std::uint32_t m_CaptureHeight = 0;
};

} // namespace Hockey
