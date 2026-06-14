#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/Editor/Gizmos/TransformGizmo.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

// Editor scene viewport. Renders the active scene through the EditorCamera into
// an offscreen target, displays it as an ImGui image, and layers grid lines,
// selection highlights, click-to-pick selection and ImGuizmo transform handles
// on top.
class SceneViewportPanel : public Panel {
public:
    SceneViewportPanel();

    void OnImGui(EditorContext& context) override;

private:
    void DrawViewport(EditorContext& context);
    void EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height);
    void UpdateCamera(EditorContext& context, bool hovered);
    void ProcessHotkeys(EditorContext& context, bool hovered);
    void Pick(EditorContext& context, const glm::vec2& viewportUV, float aspect, bool additive);

    TextureHandle m_Target{};
    std::uint32_t m_Width = 0;
    std::uint32_t m_Height = 0;

    TransformGizmo m_Gizmo;

    // True while the user is actively navigating (look/pan/orbit) with a drag
    // that began over this viewport, so navigation continues even if the cursor
    // leaves the panel mid-drag.
    bool m_Navigating = false;
};

} // namespace Hockey
