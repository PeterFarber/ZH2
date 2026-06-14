#include "Hockey/Renderer/Vulkan/VulkanInstance.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include <SDL3/SDL_vulkan.h>

namespace Hockey {

namespace {

bool HasLayer(const std::vector<VkLayerProperties>& layers, const char* name) {
    return std::any_of(layers.begin(), layers.end(),
                       [&](const VkLayerProperties& l) { return std::strcmp(l.layerName, name) == 0; });
}

bool HasExtension(const std::vector<VkExtensionProperties>& exts, const char* name) {
    return std::any_of(exts.begin(), exts.end(),
                       [&](const VkExtensionProperties& e) { return std::strcmp(e.extensionName, name) == 0; });
}

} // namespace

Status VulkanInstance::Create(bool enableValidation, bool enableDebugUtils) {
    if (volkInitialize() != VK_SUCCESS) {
        return Status::Fail("Failed to initialize Volk (Vulkan loader not found)");
    }

    m_ApiVersion = VK_API_VERSION_1_3;
    if (vkEnumerateInstanceVersion != nullptr) {
        uint32_t supported = 0;
        if (vkEnumerateInstanceVersion(&supported) == VK_SUCCESS && supported < m_ApiVersion) {
            m_ApiVersion = supported;
        }
    }
    HK_CORE_INFO("Vulkan instance API version: {}.{}.{}", VK_API_VERSION_MAJOR(m_ApiVersion),
                 VK_API_VERSION_MINOR(m_ApiVersion), VK_API_VERSION_PATCH(m_ApiVersion));

    // Surface extensions required by SDL for the active windowing system.
    std::vector<const char*> extensions;
    uint32_t sdlCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlCount);
    if (sdlExtensions == nullptr) {
        return Status::Fail(std::string("SDL_Vulkan_GetInstanceExtensions failed: ") + SDL_GetError());
    }
    extensions.assign(sdlExtensions, sdlExtensions + sdlCount);

    uint32_t availExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availExtCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availExtCount, availableExtensions.data());

    m_DebugUtilsEnabled = false;
    if ((enableDebugUtils || enableValidation) &&
        HasExtension(availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        m_DebugUtilsEnabled = true;
    }

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::vector<const char*> layers;
    m_ValidationEnabled = false;
    if (enableValidation) {
        if (HasLayer(availableLayers, "VK_LAYER_KHRONOS_validation")) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            m_ValidationEnabled = true;
        } else {
            HK_CORE_WARN("Validation requested but VK_LAYER_KHRONOS_validation not available");
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "HockeyGame";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "HockeyEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = m_ApiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
    volkLoadInstance(m_Instance);

    HK_CORE_INFO("Vulkan instance created ({} extensions, {} layers, validation={})", extensions.size(), layers.size(),
                 m_ValidationEnabled);
    for (const char* ext : extensions) {
        HK_CORE_TRACE("  instance extension: {}", ext);
    }
    for (const char* layer : layers) {
        HK_CORE_TRACE("  instance layer: {}", layer);
    }

    return Status::Ok();
}

void VulkanInstance::Destroy() {
    if (m_Instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
}

} // namespace Hockey
