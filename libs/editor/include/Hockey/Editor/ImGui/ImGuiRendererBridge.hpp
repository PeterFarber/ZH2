#pragma once

#include <cstdint>
#include <unordered_map>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

struct ImDrawData;

namespace Hockey {

class Renderer;

// Vulkan-side Dear ImGui integration. Owns the ImGui_ImplVulkan backend and the
// descriptor sets used to display offscreen viewport targets via ImGui::Image.
// All Vulkan usage is confined to the .cpp; this header stays Vulkan-free so the
// rest of the editor never sees Vulkan types.
class ImGuiRendererBridge {
public:
    // Initializes ImGui_ImplVulkan against the renderer's device/swapchain. The
    // ImGui context must already exist (created by ImGuiLayer).
    Status Init(Renderer& renderer);
    void Shutdown();

    void NewFrame();
    void RenderDrawData(ImDrawData* drawData);

    // Returns an ImTextureID (a VkDescriptorSet reinterpreted as ImU64) for the
    // current frame's color image of an offscreen render target, suitable for
    // ImGui::Image. Returns 0 when the target is unavailable.
    std::uint64_t ViewportTextureId(TextureHandle target);

    // Returns an ImTextureID for a cooked texture asset (by AssetID), uploading
    // it through the renderer on first use, for material/texture thumbnails.
    // Descriptors are cached by image view so a recooked texture (new view)
    // produces a fresh thumbnail. Returns 0 when the asset is unavailable.
    std::uint64_t TextureIdForAsset(std::uint64_t textureAssetId);

    // Releases all cached viewport descriptor sets. Call after the renderer has
    // rebuilt offscreen targets (e.g. on resize) so stale image views are not
    // referenced.
    void InvalidateViewportTextures();

    bool IsInitialized() const {
        return m_Initialized;
    }

private:
    Renderer* m_Renderer = nullptr;
    bool m_Initialized = false;
    // VkImageView (as u64) -> VkDescriptorSet (as u64).
    std::unordered_map<std::uint64_t, std::uint64_t> m_ViewportTextures;
    // VkImageView (as u64) -> VkDescriptorSet (as u64) for asset thumbnails.
    std::unordered_map<std::uint64_t, std::uint64_t> m_AssetTextures;
};

} // namespace Hockey
