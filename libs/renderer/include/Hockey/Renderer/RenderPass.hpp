#pragma once

#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/RenderTarget.hpp"

namespace Hockey {

// Forward-declared: the execution context is a Vulkan-aware structure defined
// in RenderContext.hpp. Keeping it opaque here means RenderPass/RenderGraph
// headers stay free of any Vulkan dependency.
struct RenderContext;

// The kind of work a pass performs. Graphics passes open a dynamic-rendering
// scope over their attachments; compute/transfer passes do not.
enum class RenderPassType { Graphics, Compute, Transfer };

// A color attachment written by a graphics pass. References a graph texture
// (transient or imported) plus its load/store behaviour and clear value.
struct ColorAttachment {
    TextureHandle texture;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
};

// The optional depth/stencil attachment of a graphics pass. An invalid
// texture handle means the pass has no depth attachment.
struct DepthAttachment {
    TextureHandle texture;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;

    bool Enabled() const {
        return texture.IsValid();
    }
};

// Declarative description of a render pass node. The graph reads `reads` and
// the attachment handles to derive dependencies and execution order; `execute`
// is invoked (inside the pass' rendering scope) to record draw/dispatch work.
struct RenderPassDesc {
    std::string name;
    RenderPassType type = RenderPassType::Graphics;

    // Textures sampled/read by this pass (creates a dependency on producers).
    std::vector<TextureHandle> reads;

    // Textures written by this pass.
    std::vector<ColorAttachment> colorAttachments;
    DepthAttachment depth;

    // Recorded when the pass executes. May be empty (e.g. a pure clear pass).
    std::function<void(RenderContext&)> execute;
};

const char* RenderPassTypeToString(RenderPassType type);

} // namespace Hockey
