#include "Hockey/Renderer/Vulkan/VulkanDevice.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <set>
#include <vector>

namespace Hockey {

namespace {

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool Complete() const {
        return graphics.has_value() && present.has_value();
    }
};

QueueFamilies FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilies result;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        const bool graphics = (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present);

        // Prefer a single family that does both.
        if (graphics && present) {
            result.graphics = i;
            result.present = i;
            return result;
        }
        if (graphics && !result.graphics.has_value()) {
            result.graphics = i;
        }
        if (present && !result.present.has_value()) {
            result.present = i;
        }
    }
    return result;
}

bool HasDeviceExtension(VkPhysicalDevice device, const char* name) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, exts.data());
    return std::any_of(exts.begin(), exts.end(),
                       [&](const VkExtensionProperties& e) { return std::strcmp(e.extensionName, name) == 0; });
}

uint64_t DeviceLocalMemory(VkPhysicalDevice device) {
    VkPhysicalDeviceMemoryProperties mem{};
    vkGetPhysicalDeviceMemoryProperties(device, &mem);
    uint64_t total = 0;
    for (uint32_t i = 0; i < mem.memoryHeapCount; ++i) {
        if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            total += mem.memoryHeaps[i].size;
        }
    }
    return total;
}

// Returns a positive score for a usable device, or 0 if unsuitable.
int ScoreDevice(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceProperties& props,
                const VkPhysicalDeviceFeatures& features) {
    if (!FindQueueFamilies(device, surface).Complete()) {
        return 0;
    }
    if (!HasDeviceExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return 0;
    }
    if (!features.samplerAnisotropy) {
        return 0;
    }

    int score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 10000;
    } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        score += 2000;
    } else {
        score += 500;
    }
    score += static_cast<int>(DeviceLocalMemory(device) / (256ull * 1024ull * 1024ull));
    return score;
}

} // namespace

Status VulkanDevice::Create(VkInstance instance, VkSurfaceKHR surface, RenderDevice& outDevice) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) {
        return Status::Fail("No Vulkan-capable GPU found");
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    VkPhysicalDevice best = VK_NULL_HANDLE;
    int bestScore = 0;
    VkPhysicalDeviceProperties bestProps{};

    HK_CORE_INFO("Enumerating {} Vulkan physical device(s):", count);
    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(device, &features);
        const int score = ScoreDevice(device, surface, props, features);
        HK_CORE_INFO("  - {} (type={}, score={})", props.deviceName, static_cast<int>(props.deviceType), score);
        if (score > bestScore) {
            bestScore = score;
            best = device;
            bestProps = props;
        }
    }

    if (best == VK_NULL_HANDLE) {
        return Status::Fail("No suitable GPU (need graphics+present queues, swapchain, anisotropy)");
    }

    HK_CORE_INFO("Selected GPU: {}", bestProps.deviceName);

    const QueueFamilies families = FindQueueFamilies(best, surface);
    const uint32_t graphicsFamily = families.graphics.value();
    const uint32_t presentFamily = families.present.value();

    // Query supported feature chain.
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features12;
    vkGetPhysicalDeviceFeatures2(best, &features2);

    if (!features13.dynamicRendering || !features13.synchronization2) {
        return Status::Fail("GPU does not support required Vulkan 1.3 dynamic rendering / synchronization2");
    }

    GPUInfo& gpu = outDevice.gpu;
    gpu.name = bestProps.deviceName;
    gpu.vendorID = bestProps.vendorID;
    gpu.deviceID = bestProps.deviceID;
    gpu.discrete = bestProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    gpu.dedicatedVideoMemory = DeviceLocalMemory(best);
    gpu.supportsTimelineSemaphore = features12.timelineSemaphore == VK_TRUE;
    gpu.supportsDescriptorIndexing = features12.descriptorIndexing == VK_TRUE;
    gpu.supportsSamplerAnisotropy = features2.features.samplerAnisotropy == VK_TRUE;
    gpu.supportsFillModeNonSolid = features2.features.fillModeNonSolid == VK_TRUE;
    gpu.maxSamplerAnisotropy = bestProps.limits.maxSamplerAnisotropy;

    // Build the enabled feature chain (only enable what we use + is supported).
    VkPhysicalDeviceVulkan13Features enable13{};
    enable13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enable13.dynamicRendering = VK_TRUE;
    enable13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features enable12{};
    enable12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enable12.pNext = &enable13;
    enable12.timelineSemaphore = features12.timelineSemaphore;

    VkPhysicalDeviceFeatures2 enableFeatures{};
    enableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enableFeatures.pNext = &enable12;
    enableFeatures.features.samplerAnisotropy = VK_TRUE;
    enableFeatures.features.fillModeNonSolid = features2.features.fillModeNonSolid;
    enableFeatures.features.sampleRateShading = features2.features.sampleRateShading;
    enableFeatures.features.depthClamp = features2.features.depthClamp;
    enableFeatures.features.wideLines = features2.features.wideLines;

    const float priority = 1.0f;
    std::set<uint32_t> uniqueFamilies{graphicsFamily, presentFamily};
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;
        queueInfos.push_back(qi);
    }

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &enableFeatures;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;

    VK_CHECK(vkCreateDevice(best, &deviceInfo, nullptr, &m_Device));
    volkLoadDevice(m_Device);

    outDevice.instance = instance;
    outDevice.physicalDevice = best;
    outDevice.device = m_Device;
    outDevice.graphicsFamily = graphicsFamily;
    outDevice.presentFamily = presentFamily;
    outDevice.properties = bestProps;
    vkGetDeviceQueue(m_Device, graphicsFamily, 0, &outDevice.graphicsQueue);
    vkGetDeviceQueue(m_Device, presentFamily, 0, &outDevice.presentQueue);

    outDevice.SetObjectName(reinterpret_cast<uint64_t>(m_Device), VK_OBJECT_TYPE_DEVICE, "HockeyDevice");

    HK_CORE_INFO("Logical device created (graphics family {}, present family {}, {:.0f} MB VRAM)", graphicsFamily,
                 presentFamily, static_cast<double>(gpu.dedicatedVideoMemory) / (1024.0 * 1024.0));
    return Status::Ok();
}

void VulkanDevice::Destroy() {
    if (m_Device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }
}

} // namespace Hockey
