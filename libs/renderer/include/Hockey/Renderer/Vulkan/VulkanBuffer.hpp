#pragma once

#include <vk_mem_alloc.h>

#include "Hockey/Renderer/Buffer.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// A VMA-backed Vulkan buffer. CPU-visible buffers stay persistently mapped.
struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* mapped = nullptr;
    BufferUsage usage = BufferUsage::Vertex;

    bool IsValid() const {
        return buffer != VK_NULL_HANDLE;
    }
};

// Creates a buffer sized to desc.size with the appropriate usage/memory flags.
VulkanBuffer CreateBuffer(const RenderDevice& device, const BufferDesc& desc);
void DestroyBuffer(const RenderDevice& device, VulkanBuffer& buffer);

// Uploads CPU data into the buffer. For CPU-visible buffers this is a memcpy;
// for device-local buffers a staging buffer + transfer is used.
void UploadBuffer(const RenderDevice& device, const VulkanCommand& commands, VulkanBuffer& buffer, const void* data,
                  size_t size, size_t offset = 0);

} // namespace Hockey
