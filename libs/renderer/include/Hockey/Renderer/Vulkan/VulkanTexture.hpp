#pragma once

#include <vk_mem_alloc.h>

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Texture.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// A VMA-backed image + default view.
struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    TextureFormat logicalFormat = TextureFormat::RGBA8;
    VkExtent2D extent{0, 0};
    uint32_t mipLevels = 1;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t usageFlags = 0;

    bool IsValid() const {
        return image != VK_NULL_HANDLE;
    }
};

VulkanTexture CreateTexture(const RenderDevice& device, const VulkanCommand& commands, const TextureDesc& desc);
void DestroyTexture(const RenderDevice& device, VulkanTexture& texture);

// Uploads tightly-packed pixel data into mip 0 and (if mipLevels > 1) generates
// the remaining mips, leaving the image in SHADER_READ_ONLY layout.
void UploadTexture(const RenderDevice& device, const VulkanCommand& commands, VulkanTexture& texture,
                   const void* pixels, size_t size);

} // namespace Hockey
