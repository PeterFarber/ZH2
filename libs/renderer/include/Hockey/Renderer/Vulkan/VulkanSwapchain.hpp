#pragma once

#include <vector>

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Owns the VkSwapchainKHR plus its images/views. Handles format/present-mode
// selection, extent clamping, recreation on resize, and minimized windows.
class VulkanSwapchain {
public:
    Status Create(const RenderDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height, bool vsync);
    Status Recreate(uint32_t width, uint32_t height, bool vsync);
    void Destroy();

    // Acquires the next image. Returns false with outNeedsRecreate=true when the
    // swapchain is out-of-date (caller should recreate and retry next frame).
    bool Acquire(VkSemaphore signalSemaphore, uint32_t& outImageIndex, bool& outNeedsRecreate);

    VkSwapchainKHR Get() const {
        return m_Swapchain;
    }
    VkFormat ColorFormat() const {
        return m_ColorFormat;
    }
    VkExtent2D Extent() const {
        return m_Extent;
    }
    uint32_t ImageCount() const {
        return static_cast<uint32_t>(m_Images.size());
    }
    VkImage Image(uint32_t index) const {
        return m_Images[index];
    }
    VkImageView ImageView(uint32_t index) const {
        return m_ImageViews[index];
    }
    bool SupportsTransferSrc() const {
        return m_SupportsTransferSrc;
    }

private:
    Status Build(uint32_t width, uint32_t height, bool vsync, VkSwapchainKHR oldSwapchain);
    void DestroyImageViews();

    const RenderDevice* m_Device = nullptr;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent{0, 0};
    bool m_SupportsTransferSrc = false;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
};

} // namespace Hockey
