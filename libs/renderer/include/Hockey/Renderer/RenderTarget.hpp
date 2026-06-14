#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/RenderTypes.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Attachment load/store operations and logical resource states used by render
// graph passes. These are backend-agnostic; the Vulkan layer maps them onto
// VkAttachmentLoadOp / VkImageLayout / barriers.
// ---------------------------------------------------------------------------

enum class LoadOp {
    Load,    // preserve existing contents
    Clear,   // clear to the attachment's clear value
    DontCare // contents undefined (fastest)
};

enum class StoreOp {
    Store,   // keep results for later passes / present
    DontCare // discard results
};

// How a graph texture is being used at a given point in the frame. Drives the
// image layout transitions the render graph inserts between passes.
enum class RenderResourceState {
    Undefined,
    ColorAttachment,
    DepthStencilAttachment,
    ShaderRead,
    TransferSrc,
    TransferDst,
    Present
};

const char* LoadOpToString(LoadOp op);
const char* StoreOpToString(StoreOp op);
const char* RenderResourceStateToString(RenderResourceState state);

// A realized offscreen target: one color image plus an optional depth image.
// Backs the editor viewport and intermediate HDR/post targets. The handles are
// renderer-owned texture handles (see Renderer::CreateRenderTarget).
struct RenderTarget {
    TextureHandle color;
    TextureHandle depth; // invalid when hasDepth == false
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat colorFormat = TextureFormat::RGBA16F;
    TextureFormat depthFormat = TextureFormat::Depth32F;
    bool hasDepth = true;

    bool IsValid() const {
        return color.IsValid() && width > 0 && height > 0;
    }
};

// Builds the texture descriptions used to allocate the color/depth images of an
// offscreen render target. Shared by Renderer::CreateRenderTarget (Step 13) and
// the render graph so allocation stays consistent.
TextureDesc MakeColorTargetTextureDesc(const RenderTargetDesc& desc);
TextureDesc MakeDepthTargetTextureDesc(const RenderTargetDesc& desc);

} // namespace Hockey
