#include "Hockey/Renderer/Vulkan/VulkanBuffer.hpp"

#include <cstring>

namespace Hockey {

namespace {

struct BufferTraits {
    VkBufferUsageFlags usage;
    bool hostVisible;
};

BufferTraits TraitsFor(BufferUsage usage, bool cpuVisible) {
    switch (usage) {
    case BufferUsage::Vertex:
        return {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, cpuVisible};
    case BufferUsage::Index:
        return {VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, cpuVisible};
    case BufferUsage::Uniform:
        return {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true};
    case BufferUsage::Storage:
        return {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, cpuVisible};
    case BufferUsage::Staging:
        return {VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true};
    case BufferUsage::Readback:
        return {VK_BUFFER_USAGE_TRANSFER_DST_BIT, true};
    }
    return {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpuVisible};
}

} // namespace

VulkanBuffer CreateBuffer(const RenderDevice& device, const BufferDesc& desc) {
    VulkanBuffer result;
    result.size = desc.size;
    result.usage = desc.usage;

    const BufferTraits traits = TraitsFor(desc.usage, desc.cpuVisible);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = traits.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (traits.hostVisible) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        if (desc.usage == BufferUsage::Readback) {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        } else {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
    }

    VmaAllocationInfo info{};
    if (vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &result.buffer, &result.allocation, &info) !=
        VK_SUCCESS) {
        HK_CORE_ERROR("Failed to allocate buffer of size {}", desc.size);
        return {};
    }
    if (traits.hostVisible) {
        result.mapped = info.pMappedData;
    }
    if (!desc.debugName.empty()) {
        device.SetObjectName(reinterpret_cast<uint64_t>(result.buffer), VK_OBJECT_TYPE_BUFFER, desc.debugName.c_str());
    }
    return result;
}

void DestroyBuffer(const RenderDevice& device, VulkanBuffer& buffer) {
    if (buffer.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(device.allocator, buffer.buffer, buffer.allocation);
        buffer = VulkanBuffer{};
    }
}

void UploadBuffer(const RenderDevice& device, const VulkanCommand& commands, VulkanBuffer& buffer, const void* data,
                  size_t size, size_t offset) {
    if (size == 0 || data == nullptr) {
        return;
    }
    if (buffer.mapped != nullptr) {
        std::memcpy(static_cast<char*>(buffer.mapped) + offset, data, size);
        return;
    }

    // Device-local: stage through a host-visible buffer.
    BufferDesc stagingDesc;
    stagingDesc.size = size;
    stagingDesc.usage = BufferUsage::Staging;
    stagingDesc.cpuVisible = true;
    stagingDesc.debugName = "StagingUpload";
    VulkanBuffer staging = CreateBuffer(device, stagingDesc);
    std::memcpy(staging.mapped, data, size);

    VkCommandBuffer cmd = commands.BeginSingleTimeCommands();
    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = offset;
    copy.size = size;
    vkCmdCopyBuffer(cmd, staging.buffer, buffer.buffer, 1, &copy);
    commands.EndSingleTimeCommands(cmd);

    DestroyBuffer(device, staging);
}

} // namespace Hockey
