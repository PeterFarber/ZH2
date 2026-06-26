#pragma once

#include <vector>

#include <RmlUi/Core/RenderInterface.h>

#include "Hockey/Renderer/UIOverlay.hpp"

namespace Hockey {

class Renderer;

class RmlUiRenderInterface final : public Rml::RenderInterface {
public:
    explicit RmlUiRenderInterface(Renderer& renderer);

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                        Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;
    void SetTransform(const Rml::Matrix4f* transform) override;

    const std::vector<UIOverlayDrawCommand>& DrawCommands() const;
    void ClearDrawCommands();

private:
    Renderer* m_Renderer = nullptr;
    bool m_ScissorEnabled = false;
    UIOverlayScissor m_Scissor;
    std::optional<glm::mat4> m_Transform;
    std::vector<UIOverlayDrawCommand> m_DrawCommands;
};

} // namespace Hockey
