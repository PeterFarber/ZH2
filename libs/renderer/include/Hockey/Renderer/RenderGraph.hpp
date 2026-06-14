#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/RenderPass.hpp"
#include "Hockey/Renderer/RenderTarget.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

// How a transient texture's size is derived. Absolute uses the desc's
// width/height verbatim; RenderAreaRelative scales with the frame's render
// area (full-res targets, half-res bloom, etc).
struct RenderGraphSizing {
    enum class Mode { Absolute, RenderAreaRelative };
    Mode mode = Mode::RenderAreaRelative;
    float scale = 1.0f;

    static RenderGraphSizing Absolute() {
        return {Mode::Absolute, 1.0f};
    }
    static RenderGraphSizing RenderArea(float scale = 1.0f) {
        return {Mode::RenderAreaRelative, scale};
    }
};

// ---------------------------------------------------------------------------
// RenderGraph
//
// A per-frame directed acyclic graph of render passes and the textures they
// produce/consume. Responsibilities:
//   * declare transient textures and import external ones (swapchain, targets)
//   * record passes with their reads/writes and an execute callback
//   * Compile(): validate dependencies, order passes, track resource lifetimes
//   * Execute(): run passes in order, letting the backend insert barriers and
//     open/close rendering scopes via RenderContext hooks
//   * Resize(): recompute render-area-relative texture extents
//
// The class is fully backend-agnostic and therefore unit-testable without a
// GPU. The Vulkan layer (VulkanRenderGraph) realizes the declared resources and
// supplies the per-pass begin/end hooks.
// ---------------------------------------------------------------------------
class RenderGraph {
public:
    // Read-only view of a graph resource, exposed for the backend and tests.
    struct Resource {
        TextureDesc desc;
        RenderGraphSizing sizing;
        bool imported = false;
        TextureHandle external; // the renderer-side handle when imported
        // Filled by Compile(): position (in execution order) of first/last use.
        int firstUse = -1;
        int lastUse = -1;
        // Set when the resource must be (re)allocated by the backend.
        bool dirty = true;
    };

    RenderGraph() = default;

    // ----- Declaration -----

    // Registers an externally-owned texture so passes may reference it. The
    // graph never allocates or frees imported resources.
    TextureHandle ImportTexture(const TextureDesc& desc, TextureHandle external = {});

    // Declares a graph-managed transient texture, realized by the backend.
    TextureHandle CreateTransientTexture(const TextureDesc& desc,
                                         RenderGraphSizing sizing = RenderGraphSizing::RenderArea());

    RenderPassHandle AddPass(const RenderPassDesc& desc);

    // ----- Lifecycle -----

    // Validates references, detects missing dependencies/cycles, computes an
    // execution order and resource lifetimes. Idempotent until the graph
    // changes.
    Status Compile();

    // Runs compiled passes in order. `context.beginPass`/`endPass` (if set)
    // wrap each pass; the pass' own execute callback records the work.
    void Execute(RenderContext& context);

    // Updates the render area and recomputes render-area-relative texture
    // extents, marking changed resources dirty so the backend recreates them.
    void Resize(uint32_t width, uint32_t height);

    // Clears the dirty flag on every resource. The backend calls this after it
    // has (re)allocated the GPU images for the graph's transient resources.
    void MarkResourcesRealized();

    // Clears all passes and resources (call at the start of each frame build).
    void Clear();

    // ----- Introspection (debug / tests) -----

    bool IsCompiled() const {
        return m_Compiled;
    }
    const std::string& LastError() const {
        return m_LastError;
    }
    size_t PassCount() const {
        return m_Passes.size();
    }
    size_t ResourceCount() const {
        return m_Resources.size();
    }
    uint32_t RenderAreaWidth() const {
        return m_RenderAreaWidth;
    }
    uint32_t RenderAreaHeight() const {
        return m_RenderAreaHeight;
    }

    // Pass indices in compiled execution order.
    const std::vector<uint32_t>& ExecutionOrder() const {
        return m_ExecutionOrder;
    }
    // Name of the pass at the given position in the execution order.
    const std::string& PassNameAtOrder(size_t orderIndex) const;

    const Resource* GetResource(TextureHandle handle) const;
    bool IsValidResource(TextureHandle handle) const;

private:
    struct PassNode {
        RenderPassDesc desc;
        std::vector<uint32_t> reads;  // resource indices
        std::vector<uint32_t> writes; // resource indices (color + depth)
    };

    uint32_t ResourceIndex(TextureHandle handle) const; // UINT32_MAX if invalid
    void RecomputeSizing(Resource& resource) const;
    void Invalidate();

    std::vector<Resource> m_Resources;
    std::vector<PassNode> m_Passes;
    std::vector<uint32_t> m_ExecutionOrder;

    uint32_t m_RenderAreaWidth = 0;
    uint32_t m_RenderAreaHeight = 0;
    bool m_Compiled = false;
    std::string m_LastError;
    std::string m_EmptyName;
};

} // namespace Hockey
