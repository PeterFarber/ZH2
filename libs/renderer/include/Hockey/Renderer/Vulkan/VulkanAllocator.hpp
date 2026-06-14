#pragma once

#include <vk_mem_alloc.h>

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Owns the VmaAllocator. VMA is driven through volk-loaded function pointers
// (no static/dynamic auto-loading).
class VulkanAllocator {
public:
    Status Create(const RenderDevice& device, uint32_t apiVersion);
    void Destroy();

    VmaAllocator Get() const {
        return m_Allocator;
    }

    // Reports currently used and budgeted device-local memory in bytes.
    void QueryMemory(uint64_t& usedBytes, uint64_t& budgetBytes) const;

private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};

} // namespace Hockey
