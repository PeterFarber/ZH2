#include "Hockey/Renderer/Vulkan/VulkanAllocator.hpp"

namespace Hockey {

Status VulkanAllocator::Create(const RenderDevice& device, uint32_t apiVersion) {
    // Populate the function table from volk's globally-loaded entry points.
    VmaVulkanFunctions vf{};
    vf.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vf.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vf.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vf.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vf.vkGetPhysicalDeviceProperties2KHR = vkGetPhysicalDeviceProperties2;
    vf.vkAllocateMemory = vkAllocateMemory;
    vf.vkFreeMemory = vkFreeMemory;
    vf.vkMapMemory = vkMapMemory;
    vf.vkUnmapMemory = vkUnmapMemory;
    vf.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vf.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vf.vkBindBufferMemory = vkBindBufferMemory;
    vf.vkBindImageMemory = vkBindImageMemory;
    vf.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vf.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vf.vkCreateBuffer = vkCreateBuffer;
    vf.vkDestroyBuffer = vkDestroyBuffer;
    vf.vkCreateImage = vkCreateImage;
    vf.vkDestroyImage = vkDestroyImage;
    vf.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vf.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vf.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vf.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vf.vkBindImageMemory2KHR = vkBindImageMemory2;
    vf.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vf.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vf.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo info{};
    info.vulkanApiVersion = apiVersion;
    info.instance = device.instance;
    info.physicalDevice = device.physicalDevice;
    info.device = device.device;
    info.pVulkanFunctions = &vf;

    VK_CHECK(vmaCreateAllocator(&info, &m_Allocator));
    HK_CORE_INFO("VMA allocator created");
    return Status::Ok();
}

void VulkanAllocator::Destroy() {
    if (m_Allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;
    }
}

void VulkanAllocator::QueryMemory(uint64_t& usedBytes, uint64_t& budgetBytes) const {
    usedBytes = 0;
    budgetBytes = 0;
    if (m_Allocator == VK_NULL_HANDLE) {
        return;
    }
    VmaBudget heapBudgets[VK_MAX_MEMORY_HEAPS]{};
    vmaGetHeapBudgets(m_Allocator, heapBudgets);
    for (const VmaBudget& b : heapBudgets) {
        usedBytes += b.usage;
        budgetBytes += b.budget;
    }
}

} // namespace Hockey
