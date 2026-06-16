#pragma once

// Editor-only integration seam between the Vulkan renderer and the editor's
// Dear ImGui backend. This is the ONLY public renderer header that exposes
// Vulkan types, and it is included exclusively by hockey_editor's ImGui bridge
// (never by the game client). It includes <vulkan/vulkan.h> for types only; the
// renderer translation unit that implements these functions already pulls in
// volk, so the handle typedefs are identical in both worlds.

#include <cstdint>
#include <functional>

#include <vulkan/vulkan.h>

#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

class Renderer;

// Vulkan objects required to initialize ImGui_ImplVulkan against the renderer's
// device. All handles are owned by the renderer; the editor must not destroy
// them.
struct RendererImGuiVulkanInfo {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t minImageCount = 2;
    uint32_t imageCount = 2;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    bool valid = false;
};

// A sampleable color image (current frame) for an offscreen render target, ready
// to be bound through ImGui_ImplVulkan_AddTexture for ImGui::Image().
struct RendererSampledImage {
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    bool valid = false;
};

// Static accessors into the renderer's internals. RendererImGuiAccess is a
// friend of Renderer; these are the only entry points the editor uses.
struct RendererImGuiAccess {
    // Vulkan handles for ImGui_ImplVulkan_Init. valid == false until the
    // renderer has a device + swapchain.
    static RendererImGuiVulkanInfo VulkanInfo(Renderer& renderer);

    // Current frame's sampleable color image for an offscreen target.
    static RendererSampledImage SampledImage(Renderer& renderer, TextureHandle target);

    // Sampleable image for a cooked texture asset (by AssetID), uploading it
    // through the renderer's asset texture cache on first use. Used to draw
    // material/texture thumbnails in the editor. valid == false for id 0, a
    // missing asset manager, or an unloadable texture.
    static RendererSampledImage SampledImageForAsset(Renderer& renderer, uint64_t textureAssetId);

    // Begins a dynamic-rendering pass on the acquired swapchain image (clearing
    // it to the editor background), invokes record(commandBuffer) to record the
    // ImGui draw data, ends the pass, and marks the frame so EndFrame presents
    // it. No-op if no frame is active or the swapchain is unavailable.
    static void RecordOverlay(Renderer& renderer, const std::function<void(VkCommandBuffer)>& record);
};

} // namespace Hockey
