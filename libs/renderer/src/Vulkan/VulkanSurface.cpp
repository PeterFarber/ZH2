#include "Hockey/Renderer/Vulkan/VulkanSurface.hpp"

#include <SDL3/SDL_vulkan.h>

namespace Hockey {

Status VulkanSurface::Create(VkInstance instance, SDL_Window* window) {
    if (window == nullptr) {
        return Status::Fail("Cannot create Vulkan surface: window is null");
    }
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &m_Surface)) {
        return Status::Fail(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
    }
    HK_CORE_INFO("Vulkan surface created");
    return Status::Ok();
}

void VulkanSurface::Destroy(VkInstance instance) {
    if (m_Surface != VK_NULL_HANDLE) {
        SDL_Vulkan_DestroySurface(instance, m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
}

} // namespace Hockey
