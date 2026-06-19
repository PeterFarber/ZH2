#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/Editor/Gizmos/TransformGizmo.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

struct CameraRenderData;

// Editor scene viewport. Renders the active scene with the same renderer quality
// as the client while keeping a Unity-style free editor camera by default. It
// layers editor-only debug geometry such as grid lines, selection outlines and
// physics colliders.
class SceneViewportPanel : public Panel {
public:
    SceneViewportPanel();

    void OnImGui(EditorContext& context) override;

private:
    void DrawViewport(EditorContext& context);
    void EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);
    void EnsureCaptureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);
    void UpdateCamera(EditorContext& context, bool hovered);
    void ProcessHotkeys(EditorContext& context, bool hovered);
    void Pick(EditorContext& context, const CameraRenderData& camera, const glm::vec2& viewportUV,
              const glm::vec2& viewportPixels, bool additive);

    TextureHandle m_Target{};
    std::uint32_t m_Width = 0;
    std::uint32_t m_Height = 0;
    TextureHandle m_CaptureTarget{};
    std::uint32_t m_CaptureWidth = 0;
    std::uint32_t m_CaptureHeight = 0;

    TransformGizmo m_Gizmo;

    // True while the user is actively navigating (look/pan/orbit) with a drag
    // that began over this viewport, so navigation continues even if the cursor
    // leaves the panel mid-drag.
    bool m_Navigating = false;
    bool m_FollowGameCamera = false;
};

} // namespace Hockey
