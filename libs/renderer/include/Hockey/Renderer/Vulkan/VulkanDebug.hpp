#pragma once

#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Wraps the VK_EXT_debug_utils messenger plus object-naming / labeling helpers.
class VulkanDebug {
public:
    Status Create(VkInstance instance);
    void Destroy(VkInstance instance);

    static bool IsAvailable();

private:
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
};

// Assigns a human-readable name to a Vulkan object (visible in validation
// messages and RenderDoc). No-ops when debug utils is unavailable.
void SetObjectName(VkDevice device, uint64_t handle, VkObjectType type, const char* name);

// Scoped command-buffer debug labels for grouping work in capture tools.
void BeginDebugLabel(VkCommandBuffer cmd, const char* name, float r = 0.6f, float g = 0.6f, float b = 0.9f);
void EndDebugLabel(VkCommandBuffer cmd);
void InsertDebugLabel(VkCommandBuffer cmd, const char* name);

} // namespace Hockey
