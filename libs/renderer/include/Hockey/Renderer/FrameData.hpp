#pragma once

#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Per-frame-in-flight CPU/GPU synchronization and recording state. The
// render-finished semaphore is tracked per swapchain image (see
// VulkanFrameData), so it is intentionally not stored here.
struct FrameData {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    // Per-frame uniform buffers (camera + scene/global), filled in once the
    // buffer subsystem and descriptors are wired.
    BufferHandle cameraBuffer;
    BufferHandle sceneBuffer;
};

} // namespace Hockey
