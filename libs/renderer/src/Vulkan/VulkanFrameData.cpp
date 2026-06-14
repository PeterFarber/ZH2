#include "Hockey/Renderer/Vulkan/VulkanFrameData.hpp"

#include <string>

namespace Hockey {

Status VulkanFrameData::Create(const RenderDevice& device) {
    m_Device = &device;

    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        FrameData& frame = m_Frames[i];

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = device.graphicsFamily;
        VK_CHECK(vkCreateCommandPool(device.device, &poolInfo, nullptr, &frame.commandPool));
        device.SetObjectName(reinterpret_cast<uint64_t>(frame.commandPool), VK_OBJECT_TYPE_COMMAND_POOL,
                             ("FrameCommandPool" + std::to_string(i)).c_str());

        VkCommandBufferAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool = frame.commandPool;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device.device, &alloc, &frame.commandBuffer));
        device.SetObjectName(reinterpret_cast<uint64_t>(frame.commandBuffer), VK_OBJECT_TYPE_COMMAND_BUFFER,
                             ("FrameCommandBuffer" + std::to_string(i)).c_str());

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(device.device, &semInfo, nullptr, &frame.imageAvailableSemaphore));

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device.device, &fenceInfo, nullptr, &frame.inFlightFence));
    }
    HK_CORE_INFO("Frame data created ({} frames in flight)", kFramesInFlight);
    return Status::Ok();
}

Status VulkanFrameData::CreatePresentSemaphores(uint32_t imageCount) {
    DestroyPresentSemaphores();
    m_RenderFinished.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(m_Device->device, &semInfo, nullptr, &m_RenderFinished[i]));
    }
    return Status::Ok();
}

void VulkanFrameData::DestroyPresentSemaphores() {
    if (m_Device == nullptr) {
        return;
    }
    for (VkSemaphore sem : m_RenderFinished) {
        vkDestroySemaphore(m_Device->device, sem, nullptr);
    }
    m_RenderFinished.clear();
}

void VulkanFrameData::Destroy() {
    if (m_Device == nullptr) {
        return;
    }
    DestroyPresentSemaphores();
    for (FrameData& frame : m_Frames) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_Device->device, frame.inFlightFence, nullptr);
        }
        if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_Device->device, frame.imageAvailableSemaphore, nullptr);
        }
        if (frame.commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device->device, frame.commandPool, nullptr);
        }
        frame = FrameData{};
    }
}

} // namespace Hockey
