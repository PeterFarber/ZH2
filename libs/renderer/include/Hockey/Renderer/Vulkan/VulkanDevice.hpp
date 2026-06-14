#pragma once

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Selects a suitable physical GPU, creates the logical device + queues, and
// populates a RenderDevice context. Requires Vulkan 1.3 (dynamic rendering and
// synchronization2 are used throughout the renderer).
class VulkanDevice {
public:
    Status Create(VkInstance instance, VkSurfaceKHR surface, RenderDevice& outDevice);
    void Destroy();

    VkDevice Get() const {
        return m_Device;
    }

private:
    VkDevice m_Device = VK_NULL_HANDLE;
};

} // namespace Hockey
