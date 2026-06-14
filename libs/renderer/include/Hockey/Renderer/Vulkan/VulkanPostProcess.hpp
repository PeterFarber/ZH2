#pragma once

#include <array>
#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDescriptor.hpp"
#include "Hockey/Renderer/Vulkan/VulkanPipeline.hpp"

namespace Hockey {

// Owns the full-screen post-processing pipelines (tone map, bloom down/up,
// SSAO + blur, FXAA) and a per-frame transient descriptor allocator used to
// bind each pass's single input texture. All pipelines share one layout: one
// per-pass combined-image-sampler set + a 128-byte fragment push range.
class VulkanPostProcess {
public:
    Status Create(const RenderDevice& device);
    Status BuildPipelines(const std::filesystem::path& shaderSrc, const std::filesystem::path& shaderBin,
                          VkFormat hdrFormat, VkFormat ldrFormat, VkFormat aoFormat, VkFormat swapchainFormat);
    void Destroy();

    // Recycles the transient per-pass sets for the given frame (call once the
    // frame's fence has been waited on, i.e. at BeginFrame).
    void BeginFrame(uint32_t frameIndex);

    // Records a full-screen pass into the currently-open rendering scope: binds
    // the pipeline + a freshly-allocated set pointing at `input`, pushes
    // `pushSize` bytes, and draws the full-screen triangle. The caller owns the
    // vkCmdBeginRendering/EndRendering scope, viewport, and barriers.
    void Draw(VkCommandBuffer cmd, uint32_t frameIndex, VkPipeline pipeline, VkImageView input, VkSampler sampler,
              const void* push, uint32_t pushSize);

    VkPipeline Tonemap() const {
        return m_Tonemap.pipeline;
    }
    VkPipeline BloomDown() const {
        return m_BloomDown.pipeline;
    }
    VkPipeline BloomUp() const {
        return m_BloomUp.pipeline;
    }
    VkPipeline Ssao() const {
        return m_Ssao.pipeline;
    }
    VkPipeline SsaoBlur() const {
        return m_SsaoBlur.pipeline;
    }
    VkPipeline Fxaa() const {
        return m_Fxaa.pipeline;
    }

private:
    const RenderDevice* m_Device = nullptr;
    VkDescriptorSetLayout m_PerPassLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;

    VulkanPipeline m_Tonemap;
    VulkanPipeline m_BloomDown;
    VulkanPipeline m_BloomUp;
    VulkanPipeline m_Ssao;
    VulkanPipeline m_SsaoBlur;
    VulkanPipeline m_Fxaa;

    std::array<VulkanDescriptorAllocator, kFramesInFlight> m_Alloc;
};

} // namespace Hockey
