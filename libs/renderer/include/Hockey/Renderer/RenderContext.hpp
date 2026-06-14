#pragma once

#include <cstdint>
#include <functional>

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/RenderPass.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

class VulkanRenderGraph;

// Per-frame execution context handed to render passes. This is an internal
// renderer header (it exposes Vulkan handles) and is never included by the
// public Renderer.hpp. Pass callbacks record draw/dispatch commands into `cmd`.
//
// The render graph drives execution backend-agnostically: it calls `beginPass`
// before each pass and `endPass` after, which the Vulkan backend populates to
// perform layout transitions and open/close the dynamic-rendering scope.
struct RenderContext {
    const RenderDevice* device = nullptr;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    uint32_t frameIndex = 0;
    VkExtent2D renderArea{0, 0};

    // Resolves graph texture handles to their realized Vulkan images/views.
    VulkanRenderGraph* resources = nullptr;

    // Optional renderer-supplied payload (scene gather results, descriptor sets,
    // pipelines). Set by the renderer before Execute; cast back inside passes.
    void* userData = nullptr;

    // Backend hooks invoked by RenderGraph::Execute around each pass. Left null
    // for CPU-only execution (tests); set by VulkanRenderGraph::Bind.
    std::function<void(RenderContext&, const RenderPassDesc&)> beginPass;
    std::function<void(RenderContext&, const RenderPassDesc&)> endPass;
};

} // namespace Hockey
