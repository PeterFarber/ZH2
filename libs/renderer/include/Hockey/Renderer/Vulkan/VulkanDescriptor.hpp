#pragma once

#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/DescriptorSet.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

VkDescriptorType ToVkDescriptorType(DescriptorType type);
VkShaderStageFlags ToVkShaderStages(uint32_t mask);

VkDescriptorSetLayout CreateDescriptorSetLayout(const RenderDevice& device, const DescriptorSetLayoutDesc& desc);
void DestroyDescriptorSetLayout(const RenderDevice& device, VkDescriptorSetLayout layout);

// The three standard layouts (global / material / per-pass).
struct VulkanDescriptorLayouts {
    VkDescriptorSetLayout global = VK_NULL_HANDLE;
    VkDescriptorSetLayout material = VK_NULL_HANDLE;
    VkDescriptorSetLayout perPass = VK_NULL_HANDLE;

    Status Create(const RenderDevice& device);
    void Destroy(const RenderDevice& device);
};

// Growable descriptor pool allocator. Allocates sets from the current pool and
// spins up a new pool when one is exhausted/fragmented.
class VulkanDescriptorAllocator {
public:
    Status Create(const RenderDevice& device, uint32_t setsPerPool = 256);
    void Destroy();

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
    // Frees every set and recycles pools (call when resources change wholesale).
    void Reset();

private:
    VkDescriptorPool CreatePool() const;

    const RenderDevice* m_Device = nullptr;
    uint32_t m_SetsPerPool = 256;
    VkDescriptorPool m_Current = VK_NULL_HANDLE;
    std::vector<VkDescriptorPool> m_UsedPools;
    std::vector<VkDescriptorPool> m_FreePools;
};

// Accumulates writes for a single descriptor set, then flushes via Update().
class DescriptorWriter {
public:
    DescriptorWriter& WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset,
                                  DescriptorType type);
    DescriptorWriter& WriteImage(uint32_t binding, VkImageView view, VkSampler sampler, VkImageLayout layout,
                                 DescriptorType type);
    void Update(const RenderDevice& device, VkDescriptorSet set);
    void Clear();

private:
    struct Record {
        uint32_t binding;
        VkDescriptorType type;
        bool isImage;
        size_t infoIndex;
    };
    // Stable storage: pointers into these are resolved at Update() time, after
    // all writes have been recorded, so reallocation during recording is safe.
    std::vector<VkDescriptorBufferInfo> m_BufferInfos;
    std::vector<VkDescriptorImageInfo> m_ImageInfos;
    std::vector<Record> m_Records;
};

} // namespace Hockey
