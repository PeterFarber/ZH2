#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"

namespace Hockey {

Status VulkanCommand::Create(const RenderDevice& device) {
    m_Device = &device;
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = device.graphicsFamily;
    VK_CHECK(vkCreateCommandPool(device.device, &info, nullptr, &m_UploadPool));
    device.SetObjectName(reinterpret_cast<uint64_t>(m_UploadPool), VK_OBJECT_TYPE_COMMAND_POOL, "UploadPool");
    return Status::Ok();
}

void VulkanCommand::Destroy() {
    if (m_UploadPool != VK_NULL_HANDLE && m_Device != nullptr) {
        vkDestroyCommandPool(m_Device->device, m_UploadPool, nullptr);
        m_UploadPool = VK_NULL_HANDLE;
    }
}

VkCommandBuffer VulkanCommand::BeginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = m_UploadPool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_Device->device, &alloc, &cmd);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    return cmd;
}

void VulkanCommand::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkCommandBufferSubmitInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdInfo.commandBuffer = commandBuffer;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;

    vkQueueSubmit2(m_Device->graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_Device->graphicsQueue);
    vkFreeCommandBuffers(m_Device->device, m_UploadPool, 1, &commandBuffer);
}

namespace {

struct LayoutState {
    VkPipelineStageFlags2 stage;
    VkAccessFlags2 access;
};

LayoutState StateForLayout(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return {VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT};
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return {VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0};
    case VK_IMAGE_LAYOUT_GENERAL:
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT};
    default:
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT};
    }
}

} // namespace

void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                     VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer,
                     uint32_t layerCount) {
    const LayoutState src = StateForLayout(oldLayout);
    const LayoutState dst = StateForLayout(newLayout);

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = src.stage;
    barrier.srcAccessMask = src.access;
    barrier.dstStageMask = dst.stage;
    barrier.dstAccessMask = dst.access;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = baseMip;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.subresourceRange.baseArrayLayer = baseLayer;
    barrier.subresourceRange.layerCount = layerCount;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);
}

} // namespace Hockey
