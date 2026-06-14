#pragma once

#include <cstdint>
#include <memory>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/Material.hpp"
#include "Hockey/Renderer/Mesh.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/RendererInitInfo.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"
#include "Hockey/Renderer/RendererStats.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

class Scene;
class DebugDraw;
class AssetManager;

// The shared Vulkan renderer. Owns all GPU resources behind opaque handles;
// no Vulkan types appear in this interface. Used by the game client and map
// editor; never linked by the dedicated server.
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Status Init(const RendererInitInfo& info);
    void Shutdown();
    bool IsInitialized() const;

    // ----- Frame lifecycle (swapchain present path) -----
    void BeginFrame();
    void RenderScene(Scene& scene, const CameraRenderData& camera);
    void EndFrame();

    void Resize(uint32_t width, uint32_t height);

    // Blocks until the GPU has finished all submitted work. Call before tearing
    // down resources that outlive the renderer (e.g. the editor's ImGui Vulkan
    // backend) so in-flight frames no longer reference them.
    void WaitIdle();

    // ----- Offscreen render targets (editor viewport) -----
    TextureHandle CreateRenderTarget(const RenderTargetDesc& desc);
    void ResizeRenderTarget(TextureHandle target, uint32_t width, uint32_t height);
    void RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target);
    // Copies an offscreen color target into the current swapchain image. Used
    // by the editor until an ImGui-based viewport exists.
    void BlitTargetToSwapchain(TextureHandle target);

    // ----- Resource creation -----
    MeshHandle CreateMesh(const MeshDesc& desc);
    TextureHandle CreateTexture(const TextureDesc& desc);
    MaterialHandle CreateMaterial(const MaterialDesc& desc);

    MeshHandle GetBuiltInMesh(BuiltInMesh mesh) const;
    MaterialHandle GetBuiltInMaterial(BuiltInMaterial material) const;

    // ----- Content pipeline integration -----
    // Supplies the asset manager used to resolve MeshRendererComponent asset
    // ids to GPU resources. The renderer loads/cooks on demand and caches the
    // resulting handles. Passing nullptr disables asset-backed rendering.
    void SetAssetManager(AssetManager* assetManager);
    // Drops the cached GPU resource for a single asset id (and, for textures,
    // any material that referenced it must also be invalidated by the caller).
    // Used by hot-reload so the next frame re-uploads the changed asset.
    void InvalidateAsset(uint64_t assetId);
    // Drops all cached asset-backed GPU resources.
    void ClearAssetCache();

    // ----- Settings / stats -----
    const RendererSettings& GetSettings() const;
    Status ApplySettings(const RendererSettings& settings);
    const RendererStats& GetStats() const;

    // ----- Debug drawing -----
    DebugDraw& Debug();

    // True once a device exists and frames can be recorded (a swapchain may be
    // temporarily unavailable while minimized).
    bool CanRender() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;

    // Editor-only integration. RendererImGuiAccess (declared in
    // RendererImGuiSupport.hpp, implemented in Renderer.cpp) reaches the
    // internal Vulkan handles needed by the editor's Dear ImGui backend. It is
    // the single, isolated seam through which Vulkan leaves the renderer.
    friend struct RendererImGuiAccess;
};

} // namespace Hockey
