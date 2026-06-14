#pragma once

#include <cstdint>
#include <string>

#include <vk_mem_alloc.h>

#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDebug.hpp"

namespace Hockey {

// Describes the selected physical GPU and its relevant capabilities.
struct GPUInfo {
    std::string name;
    uint32_t vendorID = 0;
    uint32_t deviceID = 0;
    bool discrete = false;
    uint64_t dedicatedVideoMemory = 0;

    bool supportsTimelineSemaphore = false;
    bool supportsDescriptorIndexing = false;
    bool supportsSamplerAnisotropy = false;
    bool supportsFillModeNonSolid = false;

    float maxSamplerAnisotropy = 1.0f;
};

// Aggregated GPU context shared by all renderer subsystems and resource
// wrappers. This is an internal header: it exposes Vulkan handles and is never
// included by the public Renderer.hpp (apps only see opaque handles).
struct RenderDevice {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsFamily = 0;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t presentFamily = 0;

    GPUInfo gpu;
    VkPhysicalDeviceProperties properties{};

    bool IsValid() const {
        return device != VK_NULL_HANDLE;
    }

    void SetObjectName(uint64_t handle, VkObjectType type, const char* name) const {
        ::Hockey::SetObjectName(device, handle, type, name);
    }
};

} // namespace Hockey
