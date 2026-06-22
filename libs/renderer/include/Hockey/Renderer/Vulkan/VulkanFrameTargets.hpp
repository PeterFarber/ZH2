#pragma once

#include <array>
#include <vector>

#include "Hockey/Renderer/RendererSettings.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"
#include "Hockey/Renderer/Vulkan/VulkanTexture.hpp"

namespace Hockey {

// Offscreen render targets owned per frame-in-flight: the HDR scene color, an
// LDR target for the tonemap/anti-alias hand-off, the bloom mip chain, the SSAO
// buffers, and the cascaded-shadow depth atlas. One set per frame avoids any
// hazard between concurrent in-flight frames (we wait on the frame fence before
// reusing its targets).
struct PostTargets {
    VulkanTexture hdrColor;               // RGBA16F scene color
    VulkanTexture ldrColor;               // swapchain-format tonemap output (FXAA input)
    VulkanTexture aoRaw;                  // R8 raw SSAO
    VulkanTexture aoBlur;                 // R8 blurred SSAO
    std::vector<VulkanTexture> bloomMips; // RGBA16F half-res downsample chain
    VulkanTexture shadowAtlas;            // Depth32F cascaded-shadow atlas (directional sun)
    VulkanTexture localShadowAtlas;       // Depth32F grid atlas for spot/point lights
};

// Maps the shadow quality setting to the requested atlas resolution / cascade count.
// VulkanFrameTargets clamps requested atlas sizes to the active GPU image limits
// before allocating textures.
uint32_t ShadowAtlasResolution(ShadowQuality quality);
uint32_t ShadowCascadeCount(ShadowQuality quality);
// Resolution of the shared spot/point-light shadow atlas (a grid of tiles).
uint32_t LocalShadowAtlasResolution(ShadowQuality quality);

class VulkanFrameTargets {
public:
    Status Create(const RenderDevice& device, const VulkanCommand& command);
    // (Re)builds all per-frame targets for the given render size + settings.
    Status Build(uint32_t width, uint32_t height, VkFormat swapchainFormat, const RendererSettings& settings);
    void Destroy();

    PostTargets& Frame(uint32_t index) {
        return m_Frames[index];
    }
    const VulkanTexture& ShadowPlaceholder() const {
        return m_ShadowPlaceholder;
    }
    uint32_t ShadowResolution() const {
        return m_ShadowResolution;
    }
    uint32_t LocalShadowResolution() const {
        return m_LocalShadowResolution;
    }
    uint32_t ShadowCascades() const {
        return m_ShadowCascades;
    }
    uint32_t BloomMipCount() const {
        return m_BloomMipCount;
    }
    VkExtent2D Extent() const {
        return m_Extent;
    }
    bool IsValid() const {
        return m_Extent.width > 0 && m_Extent.height > 0;
    }

private:
    void DestroyFrames();

    const RenderDevice* m_Device = nullptr;
    const VulkanCommand* m_Command = nullptr;
    std::array<PostTargets, kFramesInFlight> m_Frames{};
    VulkanTexture m_ShadowPlaceholder; // 1x1 depth cleared to 1.0 (fully lit)
    VkExtent2D m_Extent{0, 0};
    uint32_t m_ShadowResolution = 0;
    uint32_t m_LocalShadowResolution = 0;
    uint32_t m_ShadowCascades = 0;
    uint32_t m_BloomMipCount = 0;
};

} // namespace Hockey
