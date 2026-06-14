#pragma once

#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Owns the VkInstance and volk initialization. Enables SDL-required surface
// extensions and, in debug, the validation layer + debug utils extension.
class VulkanInstance {
public:
    Status Create(bool enableValidation, bool enableDebugUtils);
    void Destroy();

    VkInstance Get() const {
        return m_Instance;
    }
    uint32_t ApiVersion() const {
        return m_ApiVersion;
    }
    bool ValidationEnabled() const {
        return m_ValidationEnabled;
    }
    bool DebugUtilsEnabled() const {
        return m_DebugUtilsEnabled;
    }

private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    uint32_t m_ApiVersion = 0;
    bool m_ValidationEnabled = false;
    bool m_DebugUtilsEnabled = false;
};

} // namespace Hockey
