#include "Hockey/Renderer/Vulkan/VulkanSwapchain.hpp"

#include <algorithm>
#include <array>

namespace Hockey {

namespace {

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const VkSurfaceFormatKHR& f : formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                           : formats[0];
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes, bool vsync) {
    if (vsync) {
        return VK_PRESENT_MODE_FIFO_KHR; // always supported
    }
    const bool hasMailbox = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end();
    if (hasMailbox) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    const bool hasImmediate = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end();
    if (hasImmediate) {
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

} // namespace

Status VulkanSwapchain::Create(const RenderDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height,
                               bool vsync) {
    m_Device = &device;
    m_Surface = surface;
    return Build(width, height, vsync, VK_NULL_HANDLE);
}

Status VulkanSwapchain::Recreate(uint32_t width, uint32_t height, bool vsync) {
    VkSwapchainKHR old = m_Swapchain;
    DestroyImageViews();
    const Status status = Build(width, height, vsync, old);
    if (old != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_Device->device, old, nullptr);
    }
    return status;
}

Status VulkanSwapchain::Build(uint32_t width, uint32_t height, bool vsync, VkSwapchainKHR oldSwapchain) {
    VkSurfaceCapabilitiesKHR caps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->physicalDevice, m_Surface, &caps));

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->physicalDevice, m_Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->physicalDevice, m_Surface, &formatCount, formats.data());

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->physicalDevice, m_Surface, &presentCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->physicalDevice, m_Surface, &presentCount, presentModes.data());

    const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(formats);
    const VkPresentModeKHR presentMode = ChoosePresentMode(presentModes, vsync);

    VkExtent2D extent;
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        extent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    m_Extent = extent;
    m_ColorFormat = surfaceFormat.format;

    // Zero extent (minimized) - defer real creation; caller skips frames.
    if (extent.width == 0 || extent.height == 0) {
        m_Swapchain = oldSwapchain != VK_NULL_HANDLE ? m_Swapchain : VK_NULL_HANDLE;
        return Status::Ok();
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = m_Surface;
    info.minImageCount = imageCount;
    info.imageFormat = surfaceFormat.format;
    info.imageColorSpace = surfaceFormat.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = oldSwapchain;

    const std::array<uint32_t, 2> families{m_Device->graphicsFamily, m_Device->presentFamily};
    if (m_Device->graphicsFamily != m_Device->presentFamily) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = families.data();
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateSwapchainKHR(m_Device->device, &info, nullptr, &m_Swapchain));
    m_Device->SetObjectName(reinterpret_cast<uint64_t>(m_Swapchain), VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Swapchain");

    uint32_t actualCount = 0;
    vkGetSwapchainImagesKHR(m_Device->device, m_Swapchain, &actualCount, nullptr);
    m_Images.resize(actualCount);
    vkGetSwapchainImagesKHR(m_Device->device, m_Swapchain, &actualCount, m_Images.data());

    m_ImageViews.resize(actualCount);
    for (uint32_t i = 0; i < actualCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(m_Device->device, &viewInfo, nullptr, &m_ImageViews[i]));
        m_Device->SetObjectName(reinterpret_cast<uint64_t>(m_Images[i]), VK_OBJECT_TYPE_IMAGE, "SwapchainImage");
    }

    HK_CORE_INFO("Swapchain created: {}x{}, {} images, format={}, present={}", extent.width, extent.height, actualCount,
                 static_cast<int>(surfaceFormat.format), static_cast<int>(presentMode));
    return Status::Ok();
}

bool VulkanSwapchain::Acquire(VkSemaphore signalSemaphore, uint32_t& outImageIndex, bool& outNeedsRecreate) {
    outNeedsRecreate = false;
    if (m_Swapchain == VK_NULL_HANDLE) {
        outNeedsRecreate = true;
        return false;
    }
    const VkResult result = vkAcquireNextImageKHR(m_Device->device, m_Swapchain, UINT64_MAX, signalSemaphore,
                                                  VK_NULL_HANDLE, &outImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        outNeedsRecreate = true;
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        HK_CORE_ERROR("vkAcquireNextImageKHR failed: {}", VkResultString(result));
        return false;
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        outNeedsRecreate = true; // present anyway, recreate afterwards
    }
    return true;
}

void VulkanSwapchain::DestroyImageViews() {
    if (m_Device == nullptr) {
        return;
    }
    for (VkImageView view : m_ImageViews) {
        vkDestroyImageView(m_Device->device, view, nullptr);
    }
    m_ImageViews.clear();
    m_Images.clear();
}

void VulkanSwapchain::Destroy() {
    DestroyImageViews();
    if (m_Swapchain != VK_NULL_HANDLE && m_Device != nullptr) {
        vkDestroySwapchainKHR(m_Device->device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

} // namespace Hockey
