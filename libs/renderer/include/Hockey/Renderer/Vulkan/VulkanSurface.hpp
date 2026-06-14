#pragma once

#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

struct SDL_Window;

namespace Hockey {

// Creates and owns a VkSurfaceKHR for an SDL window.
class VulkanSurface {
public:
    Status Create(VkInstance instance, SDL_Window* window);
    void Destroy(VkInstance instance);

    VkSurfaceKHR Get() const {
        return m_Surface;
    }

private:
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
};

} // namespace Hockey
