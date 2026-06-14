#pragma once

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Manages a transient command pool for one-off GPU work (uploads, layout
// transitions during resource creation).
class VulkanCommand {
public:
    Status Create(const RenderDevice& device);
    void Destroy();

    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

private:
    const RenderDevice* m_Device = nullptr;
    VkCommandPool m_UploadPool = VK_NULL_HANDLE;
};

// Synchronization2 image layout transition. Derives reasonable stage/access
// masks from the layouts; sufficient for the renderer's transitions.
void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                     VkImageAspectFlags aspect, uint32_t baseMip = 0, uint32_t mipCount = 1, uint32_t baseLayer = 0,
                     uint32_t layerCount = 1);

} // namespace Hockey
