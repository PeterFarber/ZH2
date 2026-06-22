#include "Hockey/Renderer/Vulkan/VulkanFrameTargets.hpp"

#include <algorithm>

namespace Hockey {

namespace {

VulkanTexture MakeTarget(const RenderDevice& device, const VulkanCommand& command, uint32_t width, uint32_t height,
                         TextureFormat format, uint32_t usage, const char* name) {
    TextureDesc desc;
    desc.width = std::max(1u, width);
    desc.height = std::max(1u, height);
    desc.format = format;
    desc.usageFlags = usage;
    desc.debugName = name;
    return CreateTexture(device, command, desc);
}

// Bloom mip cap by preset so the chain length acts as a quality control.
uint32_t MaxBloomMips(GraphicsPreset preset) {
    switch (preset) {
    case GraphicsPreset::Low:
        return 3;
    case GraphicsPreset::Medium:
        return 4;
    case GraphicsPreset::High:
        return 5;
    case GraphicsPreset::Ultra:
        return 6;
    case GraphicsPreset::Custom:
        return 5;
    }
    return 5;
}

uint32_t ClampAtlasToDeviceLimit(uint32_t desired, const RenderDevice& device) {
    const uint32_t maxImageDim = std::max(1u, device.properties.limits.maxImageDimension2D);
    return std::min(std::max(1u, desired), maxImageDim);
}

} // namespace

uint32_t ShadowAtlasResolution(ShadowQuality quality) {
    switch (quality) {
    case ShadowQuality::Off:
        return 1;
    case ShadowQuality::Low:
        return 1024;
    case ShadowQuality::Medium:
        return 2048;
    case ShadowQuality::High:
        return 4096;
    case ShadowQuality::Ultra:
        return 8192;
    }
    return 2048;
}

uint32_t ShadowCascadeCount(ShadowQuality quality) {
    switch (quality) {
    case ShadowQuality::Off:
        return 0;
    case ShadowQuality::Low:
        return 2;
    case ShadowQuality::Medium:
        return 3;
    case ShadowQuality::High:
        return 4;
    case ShadowQuality::Ultra:
        return 4;
    }
    return 4;
}

uint32_t LocalShadowAtlasResolution(ShadowQuality quality) {
    // 4x4 grid of tiles; per-tile resolution is this divided by 4.
    switch (quality) {
    case ShadowQuality::Off:
        return 1;
    case ShadowQuality::Low:
        return 1024; // 256/tile
    case ShadowQuality::Medium:
        return 2048; // 512/tile
    case ShadowQuality::High:
        return 2048; // 512/tile
    case ShadowQuality::Ultra:
        return 4096; // 1024/tile
    }
    return 2048;
}

Status VulkanFrameTargets::Create(const RenderDevice& device, const VulkanCommand& command) {
    m_Device = &device;
    m_Command = &command;

    // 1x1 depth placeholder cleared to 1.0 (fully lit) for the shadows-off case.
    m_ShadowPlaceholder =
        MakeTarget(device, command, 1, 1, TextureFormat::Depth32F,
                   TextureUsage_Sampled | TextureUsage_DepthStencil | TextureUsage_TransferDst, "ShadowPlaceholder");
    if (!m_ShadowPlaceholder.IsValid()) {
        return Status::Fail("Failed to create shadow placeholder");
    }

    VkCommandBuffer cmd = command.BeginSingleTimeCommands();
    TransitionImage(cmd, m_ShadowPlaceholder.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
    VkClearDepthStencilValue clear{1.0f, 0};
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    range.levelCount = 1;
    range.layerCount = 1;
    vkCmdClearDepthStencilImage(cmd, m_ShadowPlaceholder.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1,
                                &range);
    TransitionImage(cmd, m_ShadowPlaceholder.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    command.EndSingleTimeCommands(cmd);
    m_ShadowPlaceholder.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return Status::Ok();
}

Status VulkanFrameTargets::Build(uint32_t width, uint32_t height, VkFormat /*swapchainFormat*/,
                                 const RendererSettings& settings) {
    if (m_Device == nullptr) {
        return Status::Fail("VulkanFrameTargets not created");
    }
    DestroyFrames();

    if (width == 0 || height == 0) {
        m_Extent = {0, 0};
        return Status::Ok();
    }
    m_Extent = {width, height};
    m_ShadowCascades = ShadowCascadeCount(settings.shadowQuality);
    m_ShadowResolution = ClampAtlasToDeviceLimit(ShadowAtlasResolution(settings.shadowQuality), *m_Device);
    m_LocalShadowResolution = ClampAtlasToDeviceLimit(LocalShadowAtlasResolution(settings.shadowQuality), *m_Device);

    // Bloom chain: half-res then halving until small, capped by preset quality.
    const uint32_t maxMips = MaxBloomMips(settings.preset);
    m_BloomMipCount = 0;
    for (uint32_t mw = width / 2, mh = height / 2; mw >= 8 && mh >= 8 && m_BloomMipCount < maxMips; mw /= 2, mh /= 2) {
        ++m_BloomMipCount;
    }

    const RenderDevice& device = *m_Device;
    const VulkanCommand& command = *m_Command;
    for (PostTargets& frame : m_Frames) {
        frame.hdrColor = MakeTarget(device, command, width, height, TextureFormat::RGBA16F,
                                    TextureUsage_Sampled | TextureUsage_RenderTarget, "HDRColor");
        frame.ldrColor = MakeTarget(device, command, width, height, TextureFormat::RGBA8,
                                    TextureUsage_Sampled | TextureUsage_RenderTarget, "LDRColor");
        frame.aoRaw = MakeTarget(device, command, width, height, TextureFormat::R8,
                                 TextureUsage_Sampled | TextureUsage_RenderTarget, "AORaw");
        frame.aoBlur = MakeTarget(device, command, width, height, TextureFormat::R8,
                                  TextureUsage_Sampled | TextureUsage_RenderTarget, "AOBlur");

        frame.bloomMips.clear();
        frame.bloomMips.reserve(m_BloomMipCount);
        uint32_t mw = width;
        uint32_t mh = height;
        for (uint32_t i = 0; i < m_BloomMipCount; ++i) {
            mw = std::max(1u, mw / 2);
            mh = std::max(1u, mh / 2);
            frame.bloomMips.push_back(MakeTarget(device, command, mw, mh, TextureFormat::RGBA16F,
                                                 TextureUsage_Sampled | TextureUsage_RenderTarget, "BloomMip"));
        }

        frame.shadowAtlas = MakeTarget(device, command, m_ShadowResolution, m_ShadowResolution, TextureFormat::Depth32F,
                                       TextureUsage_Sampled | TextureUsage_DepthStencil, "ShadowAtlas");
        frame.localShadowAtlas =
            MakeTarget(device, command, m_LocalShadowResolution, m_LocalShadowResolution, TextureFormat::Depth32F,
                       TextureUsage_Sampled | TextureUsage_DepthStencil, "LocalShadowAtlas");

        if (!frame.hdrColor.IsValid() || !frame.ldrColor.IsValid() || !frame.aoRaw.IsValid() ||
            !frame.aoBlur.IsValid() || !frame.shadowAtlas.IsValid() || !frame.localShadowAtlas.IsValid()) {
            return Status::Fail("Failed to create offscreen frame targets");
        }
        for (const VulkanTexture& mip : frame.bloomMips) {
            if (!mip.IsValid()) {
                return Status::Fail("Failed to create bloom mip target");
            }
        }
    }

    // The local shadow atlas is bound (binding 4) and statically referenced by
    // the PBR shader every frame, but its render pass only runs when a spot/point
    // light actually casts a shadow. Put it in a sampleable layout up front so
    // skipped frames never leave it UNDEFINED under a bound descriptor.
    VkCommandBuffer cmd = command.BeginSingleTimeCommands();
    for (PostTargets& frame : m_Frames) {
        TransitionImage(cmd, frame.localShadowAtlas.image, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        frame.localShadowAtlas.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    command.EndSingleTimeCommands(cmd);

    return Status::Ok();
}

void VulkanFrameTargets::DestroyFrames() {
    if (m_Device == nullptr) {
        return;
    }
    for (PostTargets& frame : m_Frames) {
        DestroyTexture(*m_Device, frame.hdrColor);
        DestroyTexture(*m_Device, frame.ldrColor);
        DestroyTexture(*m_Device, frame.aoRaw);
        DestroyTexture(*m_Device, frame.aoBlur);
        for (VulkanTexture& mip : frame.bloomMips) {
            DestroyTexture(*m_Device, mip);
        }
        frame.bloomMips.clear();
        DestroyTexture(*m_Device, frame.shadowAtlas);
        DestroyTexture(*m_Device, frame.localShadowAtlas);
    }
}

void VulkanFrameTargets::Destroy() {
    DestroyFrames();
    if (m_Device != nullptr) {
        DestroyTexture(*m_Device, m_ShadowPlaceholder);
    }
    m_Extent = {0, 0};
    m_Device = nullptr;
    m_Command = nullptr;
}

} // namespace Hockey
