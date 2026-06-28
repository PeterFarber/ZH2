#include "Hockey/Renderer/Vulkan/VulkanDescriptor.hpp"

#include <array>

namespace Hockey {

VkDescriptorType ToVkDescriptorType(DescriptorType type) {
    switch (type) {
    case DescriptorType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::UniformBufferDynamic:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case DescriptorType::CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

VkShaderStageFlags ToVkShaderStages(uint32_t mask) {
    VkShaderStageFlags flags = 0;
    if (mask & Stage_Vertex) {
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (mask & Stage_Fragment) {
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (mask & Stage_Compute) {
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return flags;
}

VkDescriptorSetLayout CreateDescriptorSetLayout(const RenderDevice& device, const DescriptorSetLayoutDesc& desc) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.bindings.size());
    for (const DescriptorBinding& b : desc.bindings) {
        VkDescriptorSetLayoutBinding vb{};
        vb.binding = b.binding;
        vb.descriptorType = ToVkDescriptorType(b.type);
        vb.descriptorCount = b.count;
        vb.stageFlags = ToVkShaderStages(b.stages);
        bindings.push_back(vb);
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.empty() ? nullptr : bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device.device, &info, nullptr, &layout) != VK_SUCCESS) {
        HK_CORE_ERROR("vkCreateDescriptorSetLayout failed");
        return VK_NULL_HANDLE;
    }
    return layout;
}

void DestroyDescriptorSetLayout(const RenderDevice& device, VkDescriptorSetLayout layout) {
    if (layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.device, layout, nullptr);
    }
}

// ---------------------------------------------------------------------------
// VulkanDescriptorLayouts
// ---------------------------------------------------------------------------

Status VulkanDescriptorLayouts::Create(const RenderDevice& device) {
    VulkanDescriptorLayouts& descriptorLayouts = *this;
    descriptorLayouts.global = CreateDescriptorSetLayout(device, GlobalSetLayoutDesc());
    descriptorLayouts.material = CreateDescriptorSetLayout(device, MaterialSetLayoutDesc());
    descriptorLayouts.decal = CreateDescriptorSetLayout(device, DecalSetLayoutDesc());
    descriptorLayouts.perPass = CreateDescriptorSetLayout(device, PerPassSetLayoutDesc());
    if (descriptorLayouts.global == VK_NULL_HANDLE || descriptorLayouts.material == VK_NULL_HANDLE ||
        descriptorLayouts.decal == VK_NULL_HANDLE || descriptorLayouts.perPass == VK_NULL_HANDLE) {
        return Status::Fail("Failed to create standard descriptor set layouts");
    }
    return Status::Ok();
}

void VulkanDescriptorLayouts::Destroy(const RenderDevice& device) {
    DestroyDescriptorSetLayout(device, global);
    DestroyDescriptorSetLayout(device, material);
    DestroyDescriptorSetLayout(device, decal);
    DestroyDescriptorSetLayout(device, perPass);
    global = material = decal = perPass = VK_NULL_HANDLE;
}

// ---------------------------------------------------------------------------
// VulkanDescriptorAllocator
// ---------------------------------------------------------------------------

Status VulkanDescriptorAllocator::Create(const RenderDevice& device, uint32_t setsPerPool) {
    m_Device = &device;
    m_SetsPerPool = setsPerPool;
    m_Current = CreatePool();
    if (m_Current == VK_NULL_HANDLE) {
        return Status::Fail("Failed to create descriptor pool");
    }
    return Status::Ok();
}

VkDescriptorPool VulkanDescriptorAllocator::CreatePool() const {
    const std::array<VkDescriptorPoolSize, 4> sizes = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_SetsPerPool * 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_SetsPerPool * 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_SetsPerPool},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_SetsPerPool},
    }};

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = m_SetsPerPool;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(m_Device->device, &info, nullptr, &pool) != VK_SUCCESS) {
        HK_CORE_ERROR("vkCreateDescriptorPool failed");
        return VK_NULL_HANDLE;
    }
    return pool;
}

VkDescriptorSet VulkanDescriptorAllocator::Allocate(VkDescriptorSetLayout layout) {
    if (m_Current == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = m_Current;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(m_Device->device, &info, &set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        // Retire the current pool and grab/create a fresh one.
        m_UsedPools.push_back(m_Current);
        if (!m_FreePools.empty()) {
            m_Current = m_FreePools.back();
            m_FreePools.pop_back();
        } else {
            m_Current = CreatePool();
        }
        info.descriptorPool = m_Current;
        result = vkAllocateDescriptorSets(m_Device->device, &info, &set);
    }
    if (result != VK_SUCCESS) {
        HK_CORE_ERROR("vkAllocateDescriptorSets failed: {}", VkResultString(result));
        return VK_NULL_HANDLE;
    }
    return set;
}

void VulkanDescriptorAllocator::Reset() {
    if (m_Device == nullptr) {
        return;
    }
    for (VkDescriptorPool pool : m_UsedPools) {
        vkResetDescriptorPool(m_Device->device, pool, 0);
        m_FreePools.push_back(pool);
    }
    m_UsedPools.clear();
    if (m_Current != VK_NULL_HANDLE) {
        vkResetDescriptorPool(m_Device->device, m_Current, 0);
    }
}

void VulkanDescriptorAllocator::Destroy() {
    if (m_Device == nullptr) {
        return;
    }
    for (VkDescriptorPool pool : m_UsedPools) {
        vkDestroyDescriptorPool(m_Device->device, pool, nullptr);
    }
    for (VkDescriptorPool pool : m_FreePools) {
        vkDestroyDescriptorPool(m_Device->device, pool, nullptr);
    }
    if (m_Current != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device->device, m_Current, nullptr);
    }
    m_UsedPools.clear();
    m_FreePools.clear();
    m_Current = VK_NULL_HANDLE;
    m_Device = nullptr;
}

// ---------------------------------------------------------------------------
// DescriptorWriter
// ---------------------------------------------------------------------------

DescriptorWriter& DescriptorWriter::WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size,
                                                VkDeviceSize offset, DescriptorType type) {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = offset;
    info.range = size;
    m_BufferInfos.push_back(info);
    m_Records.push_back({binding, ToVkDescriptorType(type), false, m_BufferInfos.size() - 1, 1});
    return *this;
}

DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkImageView view, VkSampler sampler,
                                               VkImageLayout layout, DescriptorType type) {
    VkDescriptorImageInfo info{};
    info.imageView = view;
    info.sampler = sampler;
    info.imageLayout = layout;
    m_ImageInfos.push_back(info);
    m_Records.push_back({binding, ToVkDescriptorType(type), true, m_ImageInfos.size() - 1, 1});
    return *this;
}

DescriptorWriter& DescriptorWriter::WriteImageArray(uint32_t binding, const std::vector<VkDescriptorImageInfo>& images,
                                                    DescriptorType type) {
    const size_t first = m_ImageInfos.size();
    m_ImageInfos.insert(m_ImageInfos.end(), images.begin(), images.end());
    m_Records.push_back(
        {binding, ToVkDescriptorType(type), true, first, static_cast<uint32_t>(images.size())});
    return *this;
}

void DescriptorWriter::Update(const RenderDevice& device, VkDescriptorSet set) {
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(m_Records.size());
    for (const Record& rec : m_Records) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = rec.binding;
        write.descriptorCount = rec.descriptorCount;
        write.descriptorType = rec.type;
        if (rec.isImage) {
            write.pImageInfo = &m_ImageInfos[rec.infoIndex];
        } else {
            write.pBufferInfo = &m_BufferInfos[rec.infoIndex];
        }
        writes.push_back(write);
    }
    if (!writes.empty()) {
        vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void DescriptorWriter::Clear() {
    m_BufferInfos.clear();
    m_ImageInfos.clear();
    m_Records.clear();
}

} // namespace Hockey
