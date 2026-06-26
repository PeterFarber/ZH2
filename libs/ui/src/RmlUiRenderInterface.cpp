#include "Hockey/UI/RmlUiRenderInterface.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <glm/gtc/type_ptr.hpp>

#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {
namespace {

uint32_t ToGeometryHandleId(Rml::CompiledGeometryHandle handle) {
    return static_cast<uint32_t>(handle);
}

uint32_t ToTextureHandleId(Rml::TextureHandle handle) {
    return static_cast<uint32_t>(handle);
}

float ToFloatColor(Rml::byte value) {
    return static_cast<float>(value) / 255.0f;
}

} // namespace

RmlUiRenderInterface::RmlUiRenderInterface(Renderer& renderer) : m_Renderer(&renderer) {}

Rml::CompiledGeometryHandle RmlUiRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                                  Rml::Span<const int> indices) {
    if (m_Renderer == nullptr || vertices.empty() || indices.empty()) {
        return {};
    }

    UIOverlayGeometryDesc desc;
    desc.vertices.reserve(vertices.size());
    desc.indices.reserve(indices.size());
    desc.debugName = "RmlUiGeometry";

    for (const Rml::Vertex& vertex : vertices) {
        UIOverlayVertex converted;
        converted.position = {vertex.position.x, vertex.position.y};
        converted.texCoord = {vertex.tex_coord.x, vertex.tex_coord.y};
        converted.color = {ToFloatColor(vertex.colour.red),
                           ToFloatColor(vertex.colour.green),
                           ToFloatColor(vertex.colour.blue),
                           ToFloatColor(vertex.colour.alpha)};
        desc.vertices.push_back(converted);
    }

    for (const int index : indices) {
        if (index < 0) {
            return {};
        }
        desc.indices.push_back(static_cast<uint32_t>(index));
    }

    const UIOverlayGeometryHandle handle = m_Renderer->CreateUIOverlayGeometry(desc);
    return handle.IsValid() ? static_cast<Rml::CompiledGeometryHandle>(handle.id) : Rml::CompiledGeometryHandle{};
}

void RmlUiRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                                          Rml::TextureHandle texture) {
    if (m_Renderer == nullptr || geometry == 0) {
        return;
    }

    UIOverlayDrawCommand command;
    command.geometry = UIOverlayGeometryHandle{ToGeometryHandleId(geometry)};
    command.texture = UIOverlayTextureHandle{ToTextureHandleId(texture)};
    command.translation = {translation.x, translation.y};
    command.transform = m_Transform;
    if (m_ScissorEnabled) {
        command.scissor = m_Scissor;
    }

    m_DrawCommands.push_back(command);
    m_Renderer->RenderUIOverlay(m_DrawCommands);
}

void RmlUiRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    if (m_Renderer == nullptr || geometry == 0) {
        return;
    }
    m_Renderer->ReleaseUIOverlayGeometry(UIOverlayGeometryHandle{ToGeometryHandleId(geometry)});
}

Rml::TextureHandle RmlUiRenderInterface::LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) {
    (void)source;
    textureDimensions = {};
    return {};
}

Rml::TextureHandle RmlUiRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                         Rml::Vector2i sourceDimensions) {
    if (m_Renderer == nullptr || sourceDimensions.x <= 0 || sourceDimensions.y <= 0) {
        return {};
    }

    UIOverlayTextureDesc desc;
    desc.width = static_cast<uint32_t>(sourceDimensions.x);
    desc.height = static_cast<uint32_t>(sourceDimensions.y);
    desc.rgba8Pixels = source.data();
    desc.rgba8ByteCount = source.size();
    desc.debugName = "RmlUiTexture";

    const UIOverlayTextureHandle handle = m_Renderer->CreateUIOverlayTexture(desc);
    return handle.IsValid() ? static_cast<Rml::TextureHandle>(handle.id) : Rml::TextureHandle{};
}

void RmlUiRenderInterface::ReleaseTexture(Rml::TextureHandle texture) {
    if (m_Renderer == nullptr || texture == 0) {
        return;
    }
    m_Renderer->ReleaseUIOverlayTexture(UIOverlayTextureHandle{ToTextureHandleId(texture)});
}

void RmlUiRenderInterface::EnableScissorRegion(bool enable) {
    m_ScissorEnabled = enable;
}

void RmlUiRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
    m_Scissor.x = region.Left();
    m_Scissor.y = region.Top();
    m_Scissor.width = static_cast<uint32_t>(std::max(0, region.Width()));
    m_Scissor.height = static_cast<uint32_t>(std::max(0, region.Height()));
}

void RmlUiRenderInterface::SetTransform(const Rml::Matrix4f* transform) {
    if (transform == nullptr) {
        m_Transform.reset();
        return;
    }
    m_Transform = glm::make_mat4(transform->data());
}

const std::vector<UIOverlayDrawCommand>& RmlUiRenderInterface::DrawCommands() const {
    return m_DrawCommands;
}

void RmlUiRenderInterface::ClearDrawCommands() {
    m_DrawCommands.clear();
    if (m_Renderer != nullptr) {
        m_Renderer->RenderUIOverlay(m_DrawCommands);
    }
}

} // namespace Hockey
