#pragma once

#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/RenderGraph.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"
#include "Hockey/Renderer/Vulkan/VulkanTexture.hpp"

namespace Hockey {

struct RenderContext;

// Vulkan realization of a RenderGraph. Owns the GPU images for transient graph
// resources, tracks externally-bound (imported) images for the current frame,
// and supplies the per-pass begin/end hooks that perform layout transitions and
// open/close dynamic-rendering scopes.
class VulkanRenderGraph {
public:
    Status Create(const RenderDevice& device, const VulkanCommand& command);
    void Destroy();

    // Allocates/recreates GPU images for the graph's dirty transient resources
    // and frees any that no longer exist. Call after RenderGraph::Compile and
    // before Execute. Clears the graph's dirty flags on success.
    Status Realize(RenderGraph& graph);

    // Binds an externally-owned image (swapchain image, persistent render
    // target) to a graph resource handle for the current frame.
    void BindImported(TextureHandle handle, const VulkanTexture& texture);

    // Resolves a graph resource handle to its realized Vulkan image, or null.
    VulkanTexture* Resolve(TextureHandle handle);

    // Installs the begin/end pass hooks into a RenderContext.
    void Bind(RenderContext& context);

private:
    void BeginPass(RenderContext& context, const RenderPassDesc& desc);
    void EndPass(RenderContext& context, const RenderPassDesc& desc);

    const RenderDevice* m_Device = nullptr;
    const VulkanCommand* m_Command = nullptr;

    std::vector<VulkanTexture> m_Transient; // graph-owned images (index == id-1)
    std::vector<VulkanTexture> m_Imported;  // per-frame imported bindings
    std::vector<bool> m_IsImported;
};

} // namespace Hockey
