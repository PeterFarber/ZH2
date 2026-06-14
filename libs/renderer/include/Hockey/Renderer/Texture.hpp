#pragma once

#include <cstdint>
#include <string>

#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t usageFlags = TextureUsage_Sampled | TextureUsage_TransferDst;
    std::string debugName;

    // Optional initial pixel data (tightly packed, format-sized). May be null.
    const void* initialData = nullptr;
    size_t initialDataSize = 0;
};

// Description of an offscreen render target (color + optional depth) used for
// the editor viewport and intermediate passes.
struct RenderTargetDesc {
    uint32_t width = 1280;
    uint32_t height = 720;
    TextureFormat colorFormat = TextureFormat::RGBA16F;
    bool hasDepth = true;
    std::string debugName;
};

} // namespace Hockey
