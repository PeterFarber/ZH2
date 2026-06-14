#include "Hockey/Renderer/Vulkan/VulkanTexture.hpp"

#include <algorithm>
#include <cstring>

#include "Hockey/Renderer/Vulkan/VulkanBuffer.hpp"

namespace Hockey {

VulkanTexture CreateTexture(const RenderDevice& device, const VulkanCommand& commands, const TextureDesc& desc) {
    VulkanTexture tex;
    tex.format = ToVkFormat(desc.format);
    tex.logicalFormat = desc.format;
    tex.extent = {desc.width, desc.height};
    tex.mipLevels = std::max(1u, desc.mipLevels);
    tex.aspect = FormatAspectMask(desc.format);

    uint32_t usage = desc.usageFlags;
    if (desc.initialData != nullptr) {
        usage |= TextureUsage_TransferDst;
    }
    if (tex.mipLevels > 1) {
        usage |= TextureUsage_TransferSrc | TextureUsage_TransferDst;
    }
    tex.usageFlags = usage;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = tex.format;
    imageInfo.extent = {desc.width, desc.height, 1};
    imageInfo.mipLevels = tex.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = ToVkImageUsage(usage, desc.format);
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &tex.image, &tex.allocation, nullptr) != VK_SUCCESS) {
        HK_CORE_ERROR("Failed to allocate image {}x{}", desc.width, desc.height);
        return {};
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = tex.format;
    viewInfo.subresourceRange.aspectMask = tex.aspect;
    viewInfo.subresourceRange.levelCount = tex.mipLevels;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device.device, &viewInfo, nullptr, &tex.view) != VK_SUCCESS) {
        HK_CORE_ERROR("Failed to create image view");
        vmaDestroyImage(device.allocator, tex.image, tex.allocation);
        return {};
    }

    if (!desc.debugName.empty()) {
        device.SetObjectName(reinterpret_cast<uint64_t>(tex.image), VK_OBJECT_TYPE_IMAGE, desc.debugName.c_str());
        device.SetObjectName(reinterpret_cast<uint64_t>(tex.view), VK_OBJECT_TYPE_IMAGE_VIEW, desc.debugName.c_str());
    }

    if (desc.initialData != nullptr && desc.initialDataSize > 0) {
        UploadTexture(device, commands, tex, desc.initialData, desc.initialDataSize);
    }
    return tex;
}

void DestroyTexture(const RenderDevice& device, VulkanTexture& texture) {
    if (texture.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device.device, texture.view, nullptr);
    }
    if (texture.image != VK_NULL_HANDLE) {
        vmaDestroyImage(device.allocator, texture.image, texture.allocation);
    }
    texture = VulkanTexture{};
}

void UploadTexture(const RenderDevice& device, const VulkanCommand& commands, VulkanTexture& texture,
                   const void* pixels, size_t size) {
    BufferDesc stagingDesc;
    stagingDesc.size = size;
    stagingDesc.usage = BufferUsage::Staging;
    stagingDesc.cpuVisible = true;
    stagingDesc.debugName = "TextureStaging";
    VulkanBuffer staging = CreateBuffer(device, stagingDesc);
    std::memcpy(staging.mapped, pixels, size);

    VkCommandBuffer cmd = commands.BeginSingleTimeCommands();

    TransitionImage(cmd, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.aspect,
                    0, texture.mipLevels);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = texture.aspect;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {texture.extent.width, texture.extent.height, 1};
    vkCmdCopyBufferToImage(cmd, staging.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (texture.mipLevels > 1) {
        // Generate mips with successive blits.
        int32_t mipWidth = static_cast<int32_t>(texture.extent.width);
        int32_t mipHeight = static_cast<int32_t>(texture.extent.height);
        for (uint32_t i = 1; i < texture.mipLevels; ++i) {
            TransitionImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.aspect, i - 1, 1);

            VkImageBlit blit{};
            blit.srcSubresource.aspectMask = texture.aspect;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.layerCount = 1;
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.dstSubresource.aspectMask = texture.aspect;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.layerCount = 1;
            const int32_t nextW = mipWidth > 1 ? mipWidth / 2 : 1;
            const int32_t nextH = mipHeight > 1 ? mipHeight / 2 : 1;
            blit.dstOffsets[1] = {nextW, nextH, 1};
            vkCmdBlitImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            TransitionImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.aspect, i - 1, 1);
            mipWidth = nextW;
            mipHeight = nextH;
        }
        // Last mip is still TRANSFER_DST.
        TransitionImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.aspect, texture.mipLevels - 1, 1);
    } else {
        TransitionImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.aspect, 0, 1);
    }

    commands.EndSingleTimeCommands(cmd);
    texture.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    DestroyBuffer(device, staging);
}

} // namespace Hockey
