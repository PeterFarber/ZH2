#include "Hockey/Renderer/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/Light.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/RendererImGuiSupport.hpp"
#include "Hockey/Renderer/Vulkan/VulkanAllocator.hpp"
#include "Hockey/Renderer/Vulkan/VulkanBuffer.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDebug.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDescriptor.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanFrameData.hpp"
#include "Hockey/Renderer/Vulkan/VulkanFrameTargets.hpp"
#include "Hockey/Renderer/Vulkan/VulkanInstance.hpp"
#include "Hockey/Renderer/Vulkan/VulkanMesh.hpp"
#include "Hockey/Renderer/Vulkan/VulkanPipeline.hpp"
#include "Hockey/Renderer/Vulkan/VulkanPostProcess.hpp"
#include "Hockey/Renderer/Vulkan/VulkanSampler.hpp"
#include "Hockey/Renderer/Vulkan/VulkanShader.hpp"
#include "Hockey/Renderer/Vulkan/VulkanSurface.hpp"
#include "Hockey/Renderer/Vulkan/VulkanSwapchain.hpp"
#include "Hockey/Renderer/Vulkan/VulkanTexture.hpp"

namespace Hockey {

namespace {

// Maps a cooked material's textual alpha mode to the renderer enum.
AlphaMode AlphaModeFromString(const std::string& mode) {
    if (mode == "Mask") {
        return AlphaMode::Mask;
    }
    if (mode == "Blend") {
        return AlphaMode::Blend;
    }
    return AlphaMode::Opaque;
}

constexpr uint32_t kMaxLights = 16;

// Bloom tuning: extract HDR values above the threshold; composite the blurred
// chain back with a modest weight so highlights bleed without washing out.
constexpr float kBloomThreshold = 1.0f;
constexpr float kBloomIntensity = 0.06f;

// Offscreen render-target handles occupy a distinct id range from texture
// handles so a TextureHandle can refer to either without collision.
constexpr uint32_t kRenderTargetIdBase = 0x40000000u;

// Maps the swapchain VkFormat back to its logical TextureFormat so offscreen
// color targets can be created in the exact format the present/FXAA pipelines
// expect (the swapchain always picks one of these two sRGB formats).
TextureFormat SwapchainLogicalFormat(VkFormat format) {
    if (format == VK_FORMAT_R8G8B8A8_SRGB) {
        return TextureFormat::RGBA8_SRGB;
    }
    return TextureFormat::BGRA8_SRGB;
}

// std140-compatible GPU layouts mirroring the shader UBOs (common.glsl /
// pbr.frag). Only vec4/mat4 members are used so the layouts match without
// explicit padding.
struct CameraGPU {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 viewProj{1.0f};
    glm::vec4 position{0.0f};
    glm::vec4 clip{0.1f, 1000.0f, 0.0f, 0.0f};
};

struct GpuLight {
    glm::vec4 positionRange{0.0f};
    glm::vec4 direction{0.0f, -1.0f, 0.0f, 0.0f};
    glm::vec4 color{1.0f};
    glm::vec4 spot{1.0f, 0.9f, 0.0f, 0.0f};
};

constexpr uint32_t kMaxCascades = 4;

struct SceneGPU {
    glm::vec4 ambient{0.03f, 0.03f, 0.03f, 1.0f};
    glm::vec4 params{0.0f};        // x light count, y cascade count, z 1/atlasRes, w pcf radius
    glm::vec4 cascadeSplits{0.0f}; // view-space far distance of each cascade
    glm::mat4 cascadeViewProj[kMaxCascades];
    GpuLight lights[kMaxLights];
};

struct MaterialGPU {
    glm::vec4 baseColor{1.0f};
    glm::vec4 mrno{0.0f, 0.5f, 1.0f, 1.0f};
    glm::vec4 emissive{0.0f};
    glm::vec4 alpha{0.0f, 0.5f, 0.0f, 0.0f};
};

GpuLight PackLight(const LightRenderData& light) {
    GpuLight out;
    out.positionRange = glm::vec4(light.position, light.range);
    out.direction = glm::vec4(light.direction, static_cast<float>(light.type));
    out.color = glm::vec4(light.color, light.intensity);
    out.spot = glm::vec4(light.cosInner, light.cosOuter, 0.0f, 0.0f);
    return out;
}

// Maps a "BuiltIn.<Name>" mesh reference to its enum, or false if not built-in.
bool ParseBuiltInMesh(const std::string& name, BuiltInMesh& out) {
    constexpr const char* prefix = "BuiltIn.";
    if (name.rfind(prefix, 0) != 0) {
        return false;
    }
    const std::string suffix = name.substr(8);
    for (int i = 0; i < 8; ++i) {
        const auto mesh = static_cast<BuiltInMesh>(i);
        if (suffix == BuiltInMeshName(mesh)) {
            out = mesh;
            return true;
        }
    }
    return false;
}

bool ParseBuiltInMaterial(const std::string& name, BuiltInMaterial& out) {
    constexpr const char* prefix = "BuiltIn.";
    if (name.rfind(prefix, 0) != 0) {
        return false;
    }
    const std::string suffix = name.substr(8);
    for (int i = 0; i < 11; ++i) {
        const auto material = static_cast<BuiltInMaterial>(i);
        if (suffix == BuiltInMaterialName(material)) {
            out = material;
            return true;
        }
    }
    return false;
}

// Full-screen post-process push constants (mirror the *.frag layouts).
struct TonemapPush {
    glm::vec4 params{1.0f, 2.0f, 2.2f, 0.0f}; // x exposure, y mode, z gamma
};
struct FxaaPush {
    glm::vec4 params{0.0f}; // x = 1/w, y = 1/h, z = enabled
};
struct BloomDownPush {
    glm::vec4 texel{0.0f}; // xy = 1/srcSize, z = threshold
};
struct BloomUpPush {
    glm::vec4 params{0.0f}; // x = filter radius (uv)
};
struct SsaoPush {
    glm::vec4 params{0.1f, 1000.0f, 0.05f, 1.0f}; // near, far, radius(uv), intensity
    glm::vec4 params2{16.0f, 0.02f, 0.0f, 0.0f};  // sampleCount, bias
};
struct SsaoBlurPush {
    glm::vec4 texel{0.0f}; // xy = 1/size
};

// Directional-light cascade matrices computed by fitting each view-frustum
// slice with a light-space bounding sphere (stable size, snapped to texels).
struct CascadeResult {
    std::array<glm::mat4, kMaxCascades> viewProj{};
    std::array<float, kMaxCascades> splitFar{}; // view-space far distance
    uint32_t count = 0;
};

CascadeResult ComputeCascades(const CameraRenderData& camera, const glm::vec3& lightDir, uint32_t cascadeCount,
                              float shadowDistance, uint32_t atlasResolution) {
    CascadeResult result;
    result.count = std::min(cascadeCount, kMaxCascades);
    if (result.count == 0) {
        return result;
    }

    const float nearClip = camera.nearClip;
    const float farClip = std::min(camera.farClip, std::max(shadowDistance, nearClip + 1.0f));
    const glm::mat4 invViewProj = glm::inverse(camera.projection * camera.view);
    const glm::vec3 dir = glm::normalize(lightDir);
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    // Tile size within the 2x2 atlas; light-space units per shadow texel.
    const float tileResolution = static_cast<float>(atlasResolution) * 0.5f;

    constexpr float lambda = 0.85f; // blend uniform/log splits
    float prevSplit = nearClip;
    for (uint32_t c = 0; c < result.count; ++c) {
        const float p = static_cast<float>(c + 1) / static_cast<float>(result.count);
        const float logSplit = nearClip * std::pow(farClip / nearClip, p);
        const float uniformSplit = nearClip + (farClip - nearClip) * p;
        const float splitFar = glm::mix(uniformSplit, logSplit, lambda);

        // Frustum-slice corners: interpolate full-frustum near->far edges.
        const float fracNear = (prevSplit - nearClip) / (farClip - nearClip);
        const float fracFar = (splitFar - nearClip) / (farClip - nearClip);
        std::array<glm::vec3, 8> corners{};
        int idx = 0;
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                const glm::vec4 nearH = invViewProj * glm::vec4(x ? 1.0f : -1.0f, y ? 1.0f : -1.0f, 0.0f, 1.0f);
                const glm::vec4 farH = invViewProj * glm::vec4(x ? 1.0f : -1.0f, y ? 1.0f : -1.0f, 1.0f, 1.0f);
                const glm::vec3 nearW = glm::vec3(nearH) / nearH.w;
                const glm::vec3 farW = glm::vec3(farH) / farH.w;
                corners[idx++] = glm::mix(nearW, farW, fracNear);
                corners[idx++] = glm::mix(nearW, farW, fracFar);
            }
        }

        glm::vec3 center(0.0f);
        for (const glm::vec3& corner : corners) {
            center += corner;
        }
        center /= static_cast<float>(corners.size());
        float radius = 0.0f;
        for (const glm::vec3& corner : corners) {
            radius = std::max(radius, glm::length(corner - center));
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        const glm::vec3 eye = center - dir * radius;
        glm::mat4 lightView = glm::lookAtRH(eye, center, up);
        // Texel-snap the center in light space to reduce shimmering.
        const glm::vec3 lightSpaceCenter = glm::vec3(lightView * glm::vec4(center, 1.0f));
        const float unitsPerTexel = (2.0f * radius) / std::max(tileResolution, 1.0f);
        glm::vec3 snapped = lightSpaceCenter;
        snapped.x = std::floor(snapped.x / unitsPerTexel) * unitsPerTexel;
        snapped.y = std::floor(snapped.y / unitsPerTexel) * unitsPerTexel;
        const glm::vec3 offset = snapped - lightSpaceCenter;
        lightView[3][0] += offset.x;
        lightView[3][1] += offset.y;

        glm::mat4 lightProj = glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
        result.viewProj[c] = lightProj * lightView;
        result.splitFar[c] = splitFar;
        prevSplit = splitFar;
    }
    return result;
}

uint32_t AoSampleCount(AmbientOcclusionQuality quality) {
    switch (quality) {
    case AmbientOcclusionQuality::Off:
        return 0;
    case AmbientOcclusionQuality::Low:
        return 8;
    case AmbientOcclusionQuality::Medium:
        return 12;
    case AmbientOcclusionQuality::High:
        return 16;
    case AmbientOcclusionQuality::Ultra:
        return 24;
    }
    return 16;
}

float ToneMapperMode(ToneMapper mapper) {
    switch (mapper) {
    case ToneMapper::Linear:
        return 0.0f;
    case ToneMapper::Reinhard:
        return 1.0f;
    case ToneMapper::ACES:
        return 2.0f;
    }
    return 2.0f;
}

// Opens a dynamic-rendering scope with one color attachment (+ optional depth)
// and sets a full-target viewport/scissor. The caller must vkCmdEndRendering.
void BeginRenderingScope(VkCommandBuffer cmd, VkExtent2D extent, VkImageView color, VkAttachmentLoadOp colorLoad,
                         const std::array<float, 4>& clearValue, VkImageView depth, bool clearDepth) {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = color;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = colorLoad;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{clearValue[0], clearValue[1], clearValue[2], clearValue[3]}};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depth;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea = {{0, 0}, extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colorAttachment;
    if (depth != VK_NULL_HANDLE) {
        rendering.pDepthAttachment = &depthAttachment;
    }
    vkCmdBeginRendering(cmd, &rendering);

    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

} // namespace

struct Renderer::Impl {
    RendererSettings settings;
    RendererStats stats;
    Window* window = nullptr;

    VulkanInstance instance;
    VulkanDebug debug;
    VulkanSurface surface;
    VulkanDevice deviceObj;
    VulkanAllocator allocator;
    VulkanCommand command;
    VulkanSwapchain swapchain;
    VulkanFrameData frames;
    VulkanSamplers samplers;

    // Offscreen HDR/post targets + the full-screen post-process pipelines.
    VulkanFrameTargets targets;
    VulkanPostProcess post;

    // A material resolved to GPU resources: a per-material UBO plus a descriptor
    // set (set 1) wired to the material UBO and its PBR textures.
    struct GpuMaterial {
        MaterialDesc desc;
        VulkanBuffer ubo;
        VkDescriptorSet set = VK_NULL_HANDLE;
    };

    // A single mesh instance queued for drawing.
    struct DrawItem {
        const VulkanMesh* mesh = nullptr;
        const GpuMaterial* material = nullptr;
        glm::mat4 model{1.0f};
        float viewDepth = 0.0f; // distance from camera, for transparent sort
    };

    // Descriptor system (Step 7). Standard layouts + a growable allocator. A
    // 1x1 default texture stands in for any texture a material omits.
    VulkanDescriptorLayouts descriptorLayouts;
    VulkanDescriptorAllocator descriptorAllocator;
    VulkanTexture defaultTexture;

    // Per-frame-in-flight global uniforms + descriptor sets (camera + scene).
    std::array<VulkanBuffer, kFramesInFlight> cameraUBOs;
    std::array<VulkanBuffer, kFramesInFlight> sceneUBOs;
    std::array<VkDescriptorSet, kFramesInFlight> globalSets{};

    // Resource pools. Handle id == index + 1 (0 == invalid).
    std::vector<VulkanTexture> textures;
    std::vector<VulkanBuffer> buffers;
    std::vector<VulkanMesh> meshes;
    std::vector<GpuMaterial> materials;

    // Built-in resource handles, populated once during Init.
    std::array<MeshHandle, 8> builtInMeshes{};
    std::array<MaterialHandle, 11> builtInMaterials{};

    // Forward PBR pipelines (shared layout: set0 global, set1 material, push
    // constant model matrix). The debug-line pipeline is built for Step 13.
    VkPipelineLayout meshPipelineLayout = VK_NULL_HANDLE;
    VulkanPipeline opaquePipeline;
    VulkanPipeline transparentPipeline;
    VulkanPipeline debugLinePipeline;

    // Depth-only pipelines (position-only input, push = mat4 MVP). The shadow
    // pipeline applies depth bias; the prepass one does not (so its depth equals
    // the main pass's). Both share shadowPipelineLayout.
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
    VulkanPipeline shadowPipeline;
    VulkanPipeline depthPrepassPipeline;

    // Per-frame gathered scene data (built in GatherScene, consumed by the
    // shadow + main passes within the same frame).
    std::vector<DrawItem> frameOpaque;
    std::vector<DrawItem> frameTransparent;
    SceneGPU frameScene;
    glm::mat4 frameViewProj{1.0f};
    float frameNear = 0.1f;
    float frameFar = 1000.0f;

    // Per-frame-in-flight depth targets, sized to the swapchain and recreated on
    // resize. One per frame avoids a hazard between concurrent in-flight frames.
    std::array<VulkanTexture, kFramesInFlight> depthTargets;

    // The target set the in-progress scene render draws into. These point at the
    // swapchain-sized members by default; RenderSceneToTarget repoints them at an
    // offscreen target so the whole forward+post pipeline shares one code path.
    VulkanFrameTargets* curTargets = nullptr;
    std::array<VkDescriptorSet, kFramesInFlight>* curGlobalSets = nullptr;
    std::array<VulkanTexture, kFramesInFlight>* curDepth = nullptr;

    // Per-frame-in-flight CPU-visible vertex buffers for immediate-mode debug
    // lines; grown on demand to fit the submitted vertices.
    std::array<VulkanBuffer, kFramesInFlight> debugVertexBuffers;

    // Offscreen viewport targets (editor). Each owns a full per-frame render set
    // (intermediates + depth + global sets) plus a sampled/transferable color
    // image, so it renders at its own resolution and can be sampled or blitted.
    struct OffscreenTarget {
        bool valid = false;
        uint32_t width = 0;
        uint32_t height = 0;
        bool hasDepth = true;
        std::string debugName;
        VulkanFrameTargets intermediates;
        std::array<VulkanTexture, kFramesInFlight> color; // swapchain-format FXAA output
        std::array<VulkanTexture, kFramesInFlight> depth;
        std::array<VkDescriptorSet, kFramesInFlight> globalSets{};
    };
    std::vector<OffscreenTarget> renderTargets;

    // Current layout of the acquired swapchain image, tracked so EndFrame can
    // transition it to PRESENT regardless of whether a scene render, a blit, or
    // the clear fallback last touched it.
    VkImageLayout swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::filesystem::path shaderSourceDir;
    std::filesystem::path shaderBinaryDir;

    RenderDevice device;
    uint32_t apiVersion = 0;

    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;
    bool initialized = false;
    bool frameActive = false;
    bool sceneRendered = false;
    bool pendingResize = false;
    bool pendingTargetRebuild = false;
    uint32_t pendingWidth = 0;
    uint32_t pendingHeight = 0;

    DebugDraw debugDraw;
    std::array<float, 4> clearColor{0.02f, 0.02f, 0.05f, 1.0f};

    // Content-pipeline bridge. The asset manager is owned by the application;
    // the renderer only borrows it to resolve MeshRendererComponent asset ids to
    // GPU resources. Resolved handles are cached by asset id so each asset is
    // uploaded once. Invalid handles are cached too, so a missing/broken asset
    // is not re-cooked every frame; hot-reload clears the entry to retry.
    AssetManager* assetManager = nullptr;
    std::unordered_map<uint64_t, MeshHandle> assetMeshCache;
    std::unordered_map<uint64_t, MaterialHandle> assetMaterialCache;
    std::unordered_map<uint64_t, TextureHandle> assetTextureCache;

    void RecreateSwapchain(uint32_t width, uint32_t height);
    bool BeginRecording();
    Status CreateDepthTarget(uint32_t width, uint32_t height);
    Status BuildBuiltInPipelines();
    Status BuildMeshPipelines();
    Status SetupDescriptors();
    Status RebuildTargets();
    void UpdateGlobalImageBindings();
    void BuildBuiltInResources();
    uint32_t CreateMaterialInternal(const MaterialDesc& desc);
    VkImageView ResolveTextureView(TextureHandle handle) const;
    MeshHandle ResolveMesh(const std::string& name) const;
    MaterialHandle ResolveMaterial(const std::string& name) const;
    MeshHandle ResolveAssetMesh(uint64_t assetId);
    MaterialHandle ResolveAssetMaterial(uint64_t assetId);
    TextureHandle ResolveAssetTexture(uint64_t assetId);
    void GatherScene(Scene& scene, const CameraRenderData& camera);
    void RecordSceneDrawsInPass(VkCommandBuffer cmd);
    void RecordShadowPass(VkCommandBuffer cmd);
    void RecordAmbientOcclusion(VkCommandBuffer cmd, VulkanTexture& depth);
    void RecordBloom(VkCommandBuffer cmd, VulkanTexture& hdr);
    void RecordDebugLines(VkCommandBuffer cmd, VkImageView color, VkExtent2D extent);
    void RenderSceneInternal(Scene& scene, const CameraRenderData& camera, VkImageView outputColor,
                             VkExtent2D outputExtent);

    Status BuildOffscreenTarget(OffscreenTarget& target);
    void DestroyOffscreenTarget(OffscreenTarget& target);
    void WriteGlobalSet(VkDescriptorSet set, uint32_t frame, VulkanFrameTargets& setTargets);
    OffscreenTarget* ResolveRenderTarget(TextureHandle handle);
};

void Renderer::Impl::BuildBuiltInResources() {
    for (uint32_t i = 0; i < builtInMeshes.size(); ++i) {
        const MeshDesc meshDesc = MakeBuiltInMesh(static_cast<BuiltInMesh>(i));
        VulkanMesh gpuMesh = ::Hockey::CreateMesh(device, command, meshDesc);
        if (gpuMesh.IsValid()) {
            meshes.push_back(gpuMesh);
            builtInMeshes[i] = MeshHandle{static_cast<uint32_t>(meshes.size())};
        }
    }
    for (uint32_t i = 0; i < builtInMaterials.size(); ++i) {
        const uint32_t id = CreateMaterialInternal(MakeBuiltInMaterial(static_cast<BuiltInMaterial>(i)));
        builtInMaterials[i] = MaterialHandle{id};
    }
    stats.meshCount = static_cast<uint32_t>(meshes.size());
    stats.materialCount = static_cast<uint32_t>(materials.size());
}

VkImageView Renderer::Impl::ResolveTextureView(TextureHandle handle) const {
    if (handle.IsValid() && handle.id <= textures.size()) {
        return textures[handle.id - 1].view;
    }
    return defaultTexture.view;
}

uint32_t Renderer::Impl::CreateMaterialInternal(const MaterialDesc& desc) {
    GpuMaterial mat;
    mat.desc = desc;

    BufferDesc bufferDesc;
    bufferDesc.size = sizeof(MaterialGPU);
    bufferDesc.usage = BufferUsage::Uniform;
    bufferDesc.cpuVisible = true;
    bufferDesc.debugName = desc.debugName.empty() ? std::string("MaterialUBO") : desc.debugName + ".UBO";
    mat.ubo = CreateBuffer(device, bufferDesc);

    MaterialGPU gpu;
    gpu.baseColor = desc.baseColor;
    gpu.mrno = {desc.metallic, desc.roughness, desc.normalStrength, desc.occlusionStrength};
    gpu.emissive = glm::vec4(desc.emissiveColor, desc.emissiveStrength);
    const float mode = desc.alphaMode == AlphaMode::Opaque ? 0.0f : (desc.alphaMode == AlphaMode::Mask ? 1.0f : 2.0f);
    gpu.alpha = {mode, desc.alphaCutoff, 0.0f, 0.0f};
    if (mat.ubo.mapped != nullptr) {
        std::memcpy(mat.ubo.mapped, &gpu, sizeof(gpu));
    }

    mat.set = descriptorAllocator.Allocate(descriptorLayouts.material);
    const VkSampler sampler = samplers.Linear();
    DescriptorWriter writer;
    writer.WriteBuffer(0, mat.ubo.buffer, mat.ubo.size, 0, DescriptorType::UniformBuffer)
        .WriteImage(1, ResolveTextureView(desc.baseColorTexture), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    DescriptorType::CombinedImageSampler)
        .WriteImage(2, ResolveTextureView(desc.normalTexture), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    DescriptorType::CombinedImageSampler)
        .WriteImage(3, ResolveTextureView(desc.metallicRoughnessTexture), sampler,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
        .WriteImage(4, ResolveTextureView(desc.occlusionTexture), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    DescriptorType::CombinedImageSampler)
        .WriteImage(5, ResolveTextureView(desc.emissiveTexture), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    DescriptorType::CombinedImageSampler);
    writer.Update(device, mat.set);

    materials.push_back(mat);
    stats.materialCount = static_cast<uint32_t>(materials.size());
    return static_cast<uint32_t>(materials.size());
}

MeshHandle Renderer::Impl::ResolveMesh(const std::string& name) const {
    BuiltInMesh builtIn{};
    if (ParseBuiltInMesh(name, builtIn)) {
        return builtInMeshes[static_cast<size_t>(builtIn)];
    }
    return {};
}

MaterialHandle Renderer::Impl::ResolveMaterial(const std::string& name) const {
    BuiltInMaterial builtIn{};
    if (ParseBuiltInMaterial(name, builtIn)) {
        return builtInMaterials[static_cast<size_t>(builtIn)];
    }
    return {};
}

MeshHandle Renderer::Impl::ResolveAssetMesh(uint64_t assetId) {
    if (assetId == 0 || assetManager == nullptr) {
        return {};
    }
    const auto cached = assetMeshCache.find(assetId);
    if (cached != assetMeshCache.end()) {
        return cached->second;
    }

    MeshHandle handle;
    Result<std::shared_ptr<MeshAsset>> loaded = assetManager->Load<MeshAsset>(AssetID{assetId});
    if (loaded && loaded.value) {
        const MeshAsset& asset = *loaded.value;
        MeshDesc desc;
        desc.vertices.resize(asset.vertices.size());
        for (size_t i = 0; i < asset.vertices.size(); ++i) {
            const MeshVertex& v = asset.vertices[i];
            desc.vertices[i] = Vertex{v.position, v.normal, v.tangent, v.uv0};
        }
        desc.indices = asset.indices;
        desc.debugName = "asset:" + AssetID{assetId}.ToString();
        VulkanMesh mesh = ::Hockey::CreateMesh(device, command, desc);
        if (mesh.IsValid()) {
            meshes.push_back(mesh);
            handle = MeshHandle{static_cast<uint32_t>(meshes.size())};
            stats.meshCount = static_cast<uint32_t>(meshes.size());
        }
    }
    assetMeshCache.emplace(assetId, handle);
    return handle;
}

TextureHandle Renderer::Impl::ResolveAssetTexture(uint64_t assetId) {
    if (assetId == 0 || assetManager == nullptr) {
        return {};
    }
    const auto cached = assetTextureCache.find(assetId);
    if (cached != assetTextureCache.end()) {
        return cached->second;
    }

    TextureHandle handle;
    Result<std::shared_ptr<TextureAsset>> loaded = assetManager->Load<TextureAsset>(AssetID{assetId});
    if (loaded && loaded.value) {
        const TextureAsset& asset = *loaded.value;
        const uint32_t width = std::max(asset.width, 1u);
        const uint32_t height = std::max(asset.height, 1u);
        const size_t baseSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
        if (asset.data.size() >= baseSize) {
            TextureDesc desc;
            desc.width = width;
            desc.height = height;
            // Upload the base mip only; the cooked chain (if any) is appended
            // after it, so a single-level upload is always well-formed.
            desc.mipLevels = 1;
            desc.format =
                asset.colorSpace == TextureColorSpace::SRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
            desc.usageFlags = TextureUsage_Sampled | TextureUsage_TransferDst;
            desc.debugName = "texture:" + AssetID{assetId}.ToString();
            desc.initialData = asset.data.data();
            desc.initialDataSize = baseSize;
            VulkanTexture tex = ::Hockey::CreateTexture(device, command, desc);
            if (tex.IsValid()) {
                textures.push_back(tex);
                handle = TextureHandle{static_cast<uint32_t>(textures.size())};
                stats.textureCount = static_cast<uint32_t>(textures.size());
            }
        }
    }
    assetTextureCache.emplace(assetId, handle);
    return handle;
}

MaterialHandle Renderer::Impl::ResolveAssetMaterial(uint64_t assetId) {
    if (assetId == 0 || assetManager == nullptr) {
        return {};
    }
    const auto cached = assetMaterialCache.find(assetId);
    if (cached != assetMaterialCache.end()) {
        return cached->second;
    }

    MaterialHandle handle;
    Result<std::shared_ptr<MaterialAsset>> loaded = assetManager->Load<MaterialAsset>(AssetID{assetId});
    if (loaded && loaded.value) {
        const MaterialAsset& asset = *loaded.value;
        MaterialDesc desc;
        desc.baseColor = asset.baseColor;
        desc.metallic = asset.metallic;
        desc.roughness = asset.roughness;
        desc.normalStrength = asset.normalStrength;
        desc.occlusionStrength = asset.occlusionStrength;
        desc.emissiveColor = asset.emissiveColor;
        desc.emissiveStrength = asset.emissiveStrength;
        desc.alphaMode = AlphaModeFromString(asset.alphaMode);
        desc.alphaCutoff = asset.alphaCutoff;
        desc.baseColorTexture = ResolveAssetTexture(asset.baseColorTexture.Value());
        desc.normalTexture = ResolveAssetTexture(asset.normalTexture.Value());
        desc.metallicRoughnessTexture = ResolveAssetTexture(asset.metallicRoughnessTexture.Value());
        desc.occlusionTexture = ResolveAssetTexture(asset.occlusionTexture.Value());
        desc.emissiveTexture = ResolveAssetTexture(asset.emissiveTexture.Value());
        desc.debugName = asset.name.empty() ? ("material:" + AssetID{assetId}.ToString()) : asset.name;
        handle = MaterialHandle{CreateMaterialInternal(desc)};
    }
    assetMaterialCache.emplace(assetId, handle);
    return handle;
}

Status Renderer::Impl::SetupDescriptors() {
    if (Status s = descriptorLayouts.Create(device); !s) {
        return s;
    }
    if (Status s = descriptorAllocator.Create(device); !s) {
        return s;
    }

    // 1x1 opaque-white default texture used wherever a material omits a map.
    const uint32_t whitePixel = 0xFFFFFFFFu;
    TextureDesc texDesc;
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = TextureFormat::RGBA8;
    texDesc.usageFlags = TextureUsage_Sampled;
    texDesc.initialData = &whitePixel;
    texDesc.initialDataSize = sizeof(whitePixel);
    texDesc.debugName = "DefaultWhiteTexture";
    defaultTexture = ::Hockey::CreateTexture(device, command, texDesc);
    if (!defaultTexture.IsValid()) {
        return Status::Fail("Failed to create default texture");
    }

    // Per-frame-in-flight camera/scene UBOs + global descriptor sets. Each set
    // points at its frame's buffers, so per-frame updates are plain memcpys (no
    // descriptor rewrites) and never race the GPU.
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        BufferDesc camDesc;
        camDesc.size = sizeof(CameraGPU);
        camDesc.usage = BufferUsage::Uniform;
        camDesc.cpuVisible = true;
        camDesc.debugName = "CameraUBO";
        cameraUBOs[f] = CreateBuffer(device, camDesc);

        BufferDesc sceneDesc;
        sceneDesc.size = sizeof(SceneGPU);
        sceneDesc.usage = BufferUsage::Uniform;
        sceneDesc.cpuVisible = true;
        sceneDesc.debugName = "SceneUBO";
        sceneUBOs[f] = CreateBuffer(device, sceneDesc);

        if (!cameraUBOs[f].IsValid() || !sceneUBOs[f].IsValid()) {
            return Status::Fail("Failed to create per-frame uniform buffers");
        }

        globalSets[f] = descriptorAllocator.Allocate(descriptorLayouts.global);
        if (globalSets[f] == VK_NULL_HANDLE) {
            return Status::Fail("Failed to allocate global descriptor set");
        }

        DescriptorWriter writer;
        writer.WriteBuffer(0, cameraUBOs[f].buffer, cameraUBOs[f].size, 0, DescriptorType::UniformBuffer)
            .WriteBuffer(1, sceneUBOs[f].buffer, sceneUBOs[f].size, 0, DescriptorType::UniformBuffer);
        writer.Update(device, globalSets[f]);
    }
    UpdateGlobalImageBindings();

    return Status::Ok();
}

// Writes the shadow-map (binding 2) and SSAO (binding 3) images for one frame of
// the given target set into a global descriptor set. Uses the placeholder/white
// fallbacks when shadows/AO are disabled so the bindings are always valid.
void Renderer::Impl::WriteGlobalSet(VkDescriptorSet set, uint32_t frame, VulkanFrameTargets& setTargets) {
    if (set == VK_NULL_HANDLE) {
        return;
    }
    const bool shadowsOn = settings.shadowQuality != ShadowQuality::Off && setTargets.ShadowCascades() > 0;
    VkImageView shadowView = setTargets.ShadowPlaceholder().view;
    if (shadowsOn && setTargets.IsValid() && setTargets.Frame(frame).shadowAtlas.IsValid()) {
        shadowView = setTargets.Frame(frame).shadowAtlas.view;
    }
    // Binding 3: blurred SSAO (white when AO is disabled).
    const bool aoOn = settings.aoQuality != AmbientOcclusionQuality::Off && setTargets.IsValid();
    VkImageView aoView = defaultTexture.view;
    if (aoOn && setTargets.Frame(frame).aoBlur.IsValid()) {
        aoView = setTargets.Frame(frame).aoBlur.view;
    }
    DescriptorWriter writer;
    writer.WriteImage(2, shadowView, samplers.Shadow(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      DescriptorType::CombinedImageSampler);
    writer.WriteImage(3, aoView, samplers.LinearClamp(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      DescriptorType::CombinedImageSampler);
    writer.Update(device, set);
}

// Refreshes the swapchain global sets' image bindings. Called after the per-frame
// targets are (re)built; safe because target rebuilds follow a device-wait-idle.
void Renderer::Impl::UpdateGlobalImageBindings() {
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        WriteGlobalSet(globalSets[f], f, targets);
    }
}

Status Renderer::Impl::RebuildTargets() {
    if (swapchain.Get() == VK_NULL_HANDLE) {
        return Status::Ok();
    }
    const VkExtent2D extent = swapchain.Extent();
    if (Status s = targets.Build(extent.width, extent.height, swapchain.ColorFormat(), settings); !s) {
        return s;
    }
    if (Status s = CreateDepthTarget(extent.width, extent.height); !s) {
        return s;
    }
    UpdateGlobalImageBindings();
    // Offscreen targets share the active quality settings and depend on the
    // swapchain color format, so rebuild them alongside the swapchain targets.
    for (OffscreenTarget& t : renderTargets) {
        if (t.valid) {
            BuildOffscreenTarget(t);
        }
    }
    pendingTargetRebuild = false;
    return Status::Ok();
}

Status Renderer::Impl::BuildBuiltInPipelines() {
    if (swapchain.Get() == VK_NULL_HANDLE) {
        return Status::Ok(); // headless / minimized; nothing to build yet
    }

    ShaderDesc vsDesc;
    vsDesc.sourcePath = shaderSourceDir / "debug_line.vert";
    vsDesc.stage = ShaderStage::Vertex;
    ShaderDesc fsDesc;
    fsDesc.sourcePath = shaderSourceDir / "debug_line.frag";
    fsDesc.stage = ShaderStage::Fragment;

    Result<VulkanShaderModule> vs = LoadShaderModule(device, vsDesc, shaderBinaryDir);
    if (!vs) {
        return Status::Fail(vs.error);
    }
    Result<VulkanShaderModule> fs = LoadShaderModule(device, fsDesc, shaderBinaryDir);
    if (!fs) {
        DestroyShaderModule(device, vs.value.module);
        return Status::Fail(fs.error);
    }

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(float) * 16; // mat4 viewProj
    VkPipelineLayout layout = CreatePipelineLayout(device, {}, {pushConstant});

    GraphicsPipelineDesc desc;
    desc.vertexShader = vs.value.module;
    desc.fragmentShader = fs.value.module;
    desc.layout = layout;
    desc.vertexInput = DebugLineVertexInput();
    desc.colorFormats = {swapchain.ColorFormat()};
    desc.topology = PrimitiveTopology::LineList;
    desc.cullMode = CullMode::None;
    desc.depthTest = false;
    desc.depthWrite = false;
    desc.debugName = "DebugLinePipeline";
    debugLinePipeline = CreateGraphicsPipeline(device, desc);
    debugLinePipeline.layout = layout;
    debugLinePipeline.ownsLayout = true;

    DestroyShaderModule(device, vs.value.module);
    DestroyShaderModule(device, fs.value.module);

    if (!debugLinePipeline.IsValid()) {
        return Status::Fail("Failed to build debug-line pipeline");
    }
    return Status::Ok();
}

Status Renderer::Impl::CreateDepthTarget(uint32_t width, uint32_t height) {
    for (VulkanTexture& depth : depthTargets) {
        if (depth.IsValid()) {
            DestroyTexture(device, depth);
        }
    }
    if (width == 0 || height == 0) {
        return Status::Ok();
    }
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = TextureFormat::Depth32F;
        desc.usageFlags = TextureUsage_DepthStencil | TextureUsage_Sampled;
        desc.debugName = "SceneDepth";
        depthTargets[f] = ::Hockey::CreateTexture(device, command, desc);
        if (!depthTargets[f].IsValid()) {
            return Status::Fail("Failed to create depth target");
        }
    }
    return Status::Ok();
}

Status Renderer::Impl::BuildMeshPipelines() {
    if (swapchain.Get() == VK_NULL_HANDLE) {
        return Status::Ok(); // headless / minimized; built once a swapchain exists
    }

    ShaderDesc vsDesc;
    vsDesc.sourcePath = shaderSourceDir / "mesh.vert";
    vsDesc.stage = ShaderStage::Vertex;
    ShaderDesc fsDesc;
    fsDesc.sourcePath = shaderSourceDir / "pbr.frag";
    fsDesc.stage = ShaderStage::Fragment;

    Result<VulkanShaderModule> vs = LoadShaderModule(device, vsDesc, shaderBinaryDir);
    if (!vs) {
        return Status::Fail(vs.error);
    }
    Result<VulkanShaderModule> fs = LoadShaderModule(device, fsDesc, shaderBinaryDir);
    if (!fs) {
        DestroyShaderModule(device, vs.value.module);
        return Status::Fail(fs.error);
    }

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4); // model matrix
    meshPipelineLayout =
        CreatePipelineLayout(device, {descriptorLayouts.global, descriptorLayouts.material}, {pushConstant});
    if (meshPipelineLayout == VK_NULL_HANDLE) {
        DestroyShaderModule(device, vs.value.module);
        DestroyShaderModule(device, fs.value.module);
        return Status::Fail("Failed to create mesh pipeline layout");
    }

    const VkFormat depthFormat = ToVkFormat(TextureFormat::Depth32F);

    GraphicsPipelineDesc opaque;
    opaque.vertexShader = vs.value.module;
    opaque.fragmentShader = fs.value.module;
    opaque.layout = meshPipelineLayout;
    opaque.vertexInput = MeshVertexInput();
    opaque.colorFormats = {ToVkFormat(TextureFormat::RGBA16F)}; // render into the HDR target
    opaque.depthFormat = depthFormat;
    opaque.topology = PrimitiveTopology::TriangleList;
    opaque.cullMode = CullMode::None; // single forward pass; culling refined later
    opaque.depthTest = true;
    opaque.depthWrite = true;
    opaque.blend = BlendMode::Opaque;
    opaque.debugName = "OpaquePBRPipeline";
    opaquePipeline = CreateGraphicsPipeline(device, opaque);

    GraphicsPipelineDesc transparent = opaque;
    transparent.depthWrite = false; // read depth, do not occlude later transparents
    transparent.blend = BlendMode::AlphaBlend;
    transparent.debugName = "TransparentPBRPipeline";
    transparentPipeline = CreateGraphicsPipeline(device, transparent);

    DestroyShaderModule(device, vs.value.module);
    DestroyShaderModule(device, fs.value.module);

    if (!opaquePipeline.IsValid() || !transparentPipeline.IsValid()) {
        return Status::Fail("Failed to build PBR pipelines");
    }

    // --- Depth-only cascaded-shadow pipeline --------------------------------
    ShaderDesc shadowVsDesc;
    shadowVsDesc.sourcePath = shaderSourceDir / "shadow.vert";
    shadowVsDesc.stage = ShaderStage::Vertex;
    ShaderDesc shadowFsDesc;
    shadowFsDesc.sourcePath = shaderSourceDir / "shadow.frag";
    shadowFsDesc.stage = ShaderStage::Fragment;
    Result<VulkanShaderModule> shadowVs = LoadShaderModule(device, shadowVsDesc, shaderBinaryDir);
    if (!shadowVs) {
        return Status::Fail(shadowVs.error);
    }
    Result<VulkanShaderModule> shadowFs = LoadShaderModule(device, shadowFsDesc, shaderBinaryDir);
    if (!shadowFs) {
        DestroyShaderModule(device, shadowVs.value.module);
        return Status::Fail(shadowFs.error);
    }

    VkPushConstantRange shadowPush{};
    shadowPush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPush.offset = 0;
    shadowPush.size = sizeof(glm::mat4); // light view-proj * model
    shadowPipelineLayout = CreatePipelineLayout(device, {}, {shadowPush});

    GraphicsPipelineDesc shadow;
    shadow.vertexShader = shadowVs.value.module;
    shadow.fragmentShader = shadowFs.value.module;
    shadow.layout = shadowPipelineLayout;
    shadow.vertexInput = PositionOnlyVertexInput();
    shadow.colorFormats = {}; // depth-only
    shadow.depthFormat = depthFormat;
    shadow.topology = PrimitiveTopology::TriangleList;
    shadow.cullMode = CullMode::None;
    shadow.depthTest = true;
    shadow.depthWrite = true;
    shadow.depthBias = true; // combat shadow acne
    shadow.debugName = "ShadowPipeline";
    shadowPipeline = CreateGraphicsPipeline(device, shadow);

    // Depth prepass (for SSAO): same depth-only path, no bias so the depth it
    // writes matches the main opaque pass exactly.
    GraphicsPipelineDesc prepass = shadow;
    prepass.depthBias = false;
    prepass.debugName = "DepthPrepassPipeline";
    depthPrepassPipeline = CreateGraphicsPipeline(device, prepass);

    DestroyShaderModule(device, shadowVs.value.module);
    DestroyShaderModule(device, shadowFs.value.module);

    if (shadowPipelineLayout == VK_NULL_HANDLE || !shadowPipeline.IsValid() || !depthPrepassPipeline.IsValid()) {
        return Status::Fail("Failed to build shadow/prepass pipelines");
    }
    return Status::Ok();
}

void Renderer::Impl::RecreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device.device);
    swapchain.Recreate(width, height, settings.vsync);
    if (swapchain.Get() != VK_NULL_HANDLE) {
        frames.CreatePresentSemaphores(swapchain.ImageCount());
        RebuildTargets();
    }
    pendingResize = false;
}

Renderer::Renderer() : m_Impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() {
    Shutdown();
}

Status Renderer::Init(const RendererInitInfo& info) {
    Impl& r = *m_Impl;
    if (r.initialized) {
        return Status::Ok();
    }
    if (info.window == nullptr) {
        return Status::Fail("Renderer::Init requires a window (headless rendering not supported in Phase 3)");
    }
    r.window = info.window;
    r.settings = info.settings;
    r.shaderSourceDir = info.shaderSourceDirectory;
    r.shaderBinaryDir = info.shaderBinaryDirectory;

    if (Status s = r.instance.Create(info.enableValidation, info.enableDebugMarkers || info.enableValidation); !s) {
        return s;
    }
    if (r.instance.DebugUtilsEnabled()) {
        if (Status s = r.debug.Create(r.instance.Get()); !s) {
            return s;
        }
    }
    if (Status s = r.surface.Create(r.instance.Get(), r.window->SDLHandle()); !s) {
        return s;
    }
    if (Status s = r.deviceObj.Create(r.instance.Get(), r.surface.Get(), r.device); !s) {
        return s;
    }
    r.apiVersion = r.instance.ApiVersion();
    if (Status s = r.allocator.Create(r.device, r.apiVersion); !s) {
        return s;
    }
    r.device.allocator = r.allocator.Get();
    if (Status s = r.command.Create(r.device); !s) {
        return s;
    }
    if (Status s = r.samplers.Create(r.device, r.settings.anisotropy); !s) {
        return s;
    }
    if (Status s = r.targets.Create(r.device, r.command); !s) {
        return s;
    }
    if (Status s = r.SetupDescriptors(); !s) {
        return s;
    }
    if (Status s =
            r.swapchain.Create(r.device, r.surface.Get(), r.window->Width(), r.window->Height(), r.settings.vsync);
        !s) {
        return s;
    }
    if (Status s = r.frames.Create(r.device); !s) {
        return s;
    }
    if (Status s = r.post.Create(r.device); !s) {
        return s;
    }
    if (r.swapchain.Get() != VK_NULL_HANDLE) {
        if (Status s = r.frames.CreatePresentSemaphores(r.swapchain.ImageCount()); !s) {
            return s;
        }
        if (Status s = r.RebuildTargets(); !s) {
            return s;
        }
        if (Status s = r.post.BuildPipelines(r.shaderSourceDir, r.shaderBinaryDir, ToVkFormat(TextureFormat::RGBA16F),
                                             ToVkFormat(TextureFormat::RGBA8), ToVkFormat(TextureFormat::R8),
                                             r.swapchain.ColorFormat());
            !s) {
            return s;
        }
    }
    if (Status s = r.BuildBuiltInPipelines(); !s) {
        return s;
    }
    if (Status s = r.BuildMeshPipelines(); !s) {
        return s;
    }
    r.BuildBuiltInResources();

    r.stats.usedVRAMBytes = 0;
    r.stats.budgetVRAMBytes = 0;
    r.initialized = true;
    HK_CORE_INFO("Renderer initialized (GPU: {})", r.device.gpu.name);
    return Status::Ok();
}

void Renderer::WaitIdle() {
    Impl& r = *m_Impl;
    if (r.initialized && r.device.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(r.device.device);
    }
}

void Renderer::Shutdown() {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return;
    }
    if (r.device.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(r.device.device);
    }
    for (VulkanMesh& mesh : r.meshes) {
        DestroyMesh(r.device, mesh);
    }
    r.meshes.clear();
    for (VulkanTexture& tex : r.textures) {
        DestroyTexture(r.device, tex);
    }
    r.textures.clear();
    for (VulkanBuffer& buf : r.buffers) {
        DestroyBuffer(r.device, buf);
    }
    r.buffers.clear();
    for (Impl::GpuMaterial& mat : r.materials) {
        DestroyBuffer(r.device, mat.ubo);
    }
    r.materials.clear();
    DestroyPipeline(r.device, r.opaquePipeline);
    DestroyPipeline(r.device, r.transparentPipeline);
    DestroyPipeline(r.device, r.debugLinePipeline);
    DestroyPipeline(r.device, r.shadowPipeline);
    DestroyPipeline(r.device, r.depthPrepassPipeline);
    DestroyPipelineLayout(r.device, r.meshPipelineLayout);
    r.meshPipelineLayout = VK_NULL_HANDLE;
    DestroyPipelineLayout(r.device, r.shadowPipelineLayout);
    r.shadowPipelineLayout = VK_NULL_HANDLE;
    for (Impl::OffscreenTarget& t : r.renderTargets) {
        if (t.valid) {
            r.DestroyOffscreenTarget(t);
        }
    }
    r.renderTargets.clear();
    r.post.Destroy();
    r.targets.Destroy();
    r.descriptorAllocator.Destroy();
    r.descriptorLayouts.Destroy(r.device);
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        DestroyBuffer(r.device, r.cameraUBOs[f]);
        DestroyBuffer(r.device, r.sceneUBOs[f]);
        DestroyBuffer(r.device, r.debugVertexBuffers[f]);
    }
    for (VulkanTexture& depth : r.depthTargets) {
        DestroyTexture(r.device, depth);
    }
    DestroyTexture(r.device, r.defaultTexture);
    r.samplers.Destroy();
    r.frames.Destroy();
    r.swapchain.Destroy();
    r.command.Destroy();
    r.allocator.Destroy();
    r.deviceObj.Destroy();
    r.surface.Destroy(r.instance.Get());
    r.debug.Destroy(r.instance.Get());
    r.instance.Destroy();
    r.initialized = false;
    HK_CORE_INFO("Renderer shut down");
}

bool Renderer::IsInitialized() const {
    return m_Impl->initialized;
}

bool Renderer::CanRender() const {
    const Impl& r = *m_Impl;
    return r.initialized && r.swapchain.Get() != VK_NULL_HANDLE;
}

bool Renderer::Impl::BeginRecording() {
    FrameData& frame = frames.Frame(frameIndex);
    vkWaitForFences(device.device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);

    bool needsRecreate = false;
    if (!swapchain.Acquire(frame.imageAvailableSemaphore, imageIndex, needsRecreate)) {
        if (needsRecreate && window != nullptr) {
            RecreateSwapchain(window->Width(), window->Height());
        }
        return false;
    }
    if (needsRecreate) {
        pendingResize = true; // suboptimal: present this frame, recreate next
    }

    vkResetFences(device.device, 1, &frame.inFlightFence);
    vkResetCommandPool(device.device, frame.commandPool, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frame.commandBuffer, &begin);
    return true;
}

void Renderer::BeginFrame() {
    Impl& r = *m_Impl;
    r.frameActive = false;
    r.sceneRendered = false;
    if (!r.initialized || r.window == nullptr) {
        return;
    }

    const uint32_t width = r.window->Width();
    const uint32_t height = r.window->Height();
    if (r.window->IsMinimized() || width == 0 || height == 0) {
        return; // skip frames while minimized
    }

    if (r.pendingResize || r.swapchain.Get() == VK_NULL_HANDLE) {
        r.RecreateSwapchain(width, height);
        if (r.swapchain.Get() == VK_NULL_HANDLE) {
            return;
        }
    } else if (r.pendingTargetRebuild) {
        vkDeviceWaitIdle(r.device.device);
        r.RebuildTargets();
    }

    if (!r.BeginRecording()) {
        return;
    }

    r.stats.ResetPerFrame();
    r.post.BeginFrame(r.frameIndex);               // recycle this frame's transient post sets
    r.swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED; // freshly acquired image

    FrameData& frame = r.frames.Frame(r.frameIndex);
    BeginDebugLabel(frame.commandBuffer, "Frame");
    r.frameActive = true;
}

void Renderer::Impl::GatherScene(Scene& scene, const CameraRenderData& camera) {
    const VkExtent2D extent = curTargets->Extent();
    frameViewProj = camera.projection * camera.view;
    frameNear = camera.nearClip;
    frameFar = camera.farClip;

    // --- Per-frame camera UBO -----------------------------------------------
    CameraGPU cameraGpu;
    cameraGpu.view = camera.view;
    cameraGpu.proj = camera.projection;
    cameraGpu.viewProj = camera.projection * camera.view;
    cameraGpu.position = glm::vec4(camera.position, 1.0f);
    cameraGpu.clip =
        glm::vec4(camera.nearClip, camera.farClip, static_cast<float>(extent.width), static_cast<float>(extent.height));
    if (cameraUBOs[frameIndex].mapped != nullptr) {
        std::memcpy(cameraUBOs[frameIndex].mapped, &cameraGpu, sizeof(cameraGpu));
    }

    // --- Lights + environment -----------------------------------------------
    frameScene = SceneGPU{};
    for (glm::mat4& matrix : frameScene.cascadeViewProj) {
        matrix = glm::mat4(1.0f);
    }
    bool hasSun = false;
    glm::vec3 sunDir(0.0f, -1.0f, 0.0f);
    {
        const auto envView = scene.Registry().view<EnvironmentComponent>();
        for (const entt::entity handle : envView) {
            const Entity entity{handle, &scene};
            if (!entity.IsActiveInHierarchy()) {
                continue;
            }
            const auto& env = envView.get<EnvironmentComponent>(handle);
            frameScene.ambient = glm::vec4(env.ambientColor, env.ambientIntensity);
            break; // first active environment wins
        }

        uint32_t lightCount = 0;
        const auto lightView = scene.Registry().view<TransformComponent, LightComponent>();
        for (const entt::entity handle : lightView) {
            if (lightCount >= kMaxLights) {
                break;
            }
            const Entity entity{handle, &scene};
            if (!entity.IsActiveInHierarchy()) {
                continue;
            }
            const auto& light = lightView.get<LightComponent>(handle);
            const glm::mat4 world = scene.GetWorldTransform(entity);
            const LightRenderData data = BuildLightRenderData(world, light);
            frameScene.lights[lightCount] = PackLight(data);
            if (!hasSun && data.type == 0 && data.castsShadows) {
                hasSun = true;
                sunDir = data.direction;
            }
            ++lightCount;
        }
        frameScene.params = glm::vec4(static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f);
    }

    // --- Directional cascaded shadows ---------------------------------------
    const uint32_t cascadeCount = ShadowCascadeCount(settings.shadowQuality);
    if (settings.shadowQuality != ShadowQuality::Off && cascadeCount > 0 && hasSun && curTargets->IsValid()) {
        const uint32_t atlasRes = curTargets->ShadowResolution();
        const CascadeResult cascades = ComputeCascades(camera, sunDir, cascadeCount, settings.shadowDistance, atlasRes);
        for (uint32_t c = 0; c < cascades.count; ++c) {
            frameScene.cascadeViewProj[c] = cascades.viewProj[c];
            frameScene.cascadeSplits[c] = cascades.splitFar[c];
        }
        const float pcfRadius = settings.shadowQuality == ShadowQuality::Ultra ? 2.0f : 1.0f;
        frameScene.params.y = static_cast<float>(cascades.count);
        frameScene.params.z = 1.0f / static_cast<float>(std::max(atlasRes, 1u));
        frameScene.params.w = pcfRadius;
    }

    // --- Renderables, resolving names to handles + fallbacks ----------------
    frameOpaque.clear();
    frameTransparent.clear();
    const auto meshView = scene.Registry().view<TransformComponent, MeshRendererComponent>();
    for (const entt::entity handle : meshView) {
        const Entity entity{handle, &scene};
        if (!entity.IsActiveInHierarchy()) {
            continue;
        }
        const auto& mr = meshView.get<MeshRendererComponent>(handle);
        if (!mr.visible) {
            continue;
        }

        // Prefer the content-pipeline asset id; fall back to the built-in name,
        // then to a default mesh/error material so a bad reference still draws.
        MeshHandle meshHandle = ResolveAssetMesh(mr.meshAsset);
        if (!meshHandle.IsValid()) {
            meshHandle = ResolveMesh(mr.meshName);
        }
        if (!meshHandle.IsValid()) {
            meshHandle = builtInMeshes[static_cast<size_t>(BuiltInMesh::Cube)];
        }
        MaterialHandle materialHandle = ResolveAssetMaterial(mr.materialAsset);
        if (!materialHandle.IsValid()) {
            materialHandle = ResolveMaterial(mr.materialName);
        }
        if (!materialHandle.IsValid()) {
            materialHandle = builtInMaterials[static_cast<size_t>(BuiltInMaterial::ErrorMagenta)];
        }
        if (!meshHandle.IsValid() || meshHandle.id > meshes.size() || !materialHandle.IsValid() ||
            materialHandle.id > materials.size()) {
            continue;
        }

        DrawItem item;
        item.mesh = &meshes[meshHandle.id - 1];
        item.material = &materials[materialHandle.id - 1];
        item.model = scene.GetWorldTransform(entity);
        item.viewDepth = glm::length(glm::vec3(item.model[3]) - camera.position);
        if (!item.mesh->IsValid()) {
            continue;
        }

        if (item.material->desc.alphaMode == AlphaMode::Blend) {
            frameTransparent.push_back(item);
        } else {
            frameOpaque.push_back(item);
        }
    }

    // Transparent geometry draws back-to-front for correct blending.
    std::sort(frameTransparent.begin(), frameTransparent.end(),
              [](const DrawItem& a, const DrawItem& b) { return a.viewDepth > b.viewDepth; });
}

void Renderer::Impl::RecordSceneDrawsInPass(VkCommandBuffer cmd) {
    auto recordItems = [&](const VulkanPipeline& pipeline, const std::vector<DrawItem>& items) {
        if (items.empty() || !pipeline.IsValid()) {
            return;
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, 0, 1,
                                &(*curGlobalSets)[frameIndex], 0, nullptr);
        for (const DrawItem& item : items) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, 1, 1, &item.material->set,
                                    0, nullptr);
            vkCmdPushConstants(cmd, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &item.model);

            const VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &item.mesh->vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, item.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, item.mesh->indexCount, 1, 0, 0, 0);

            ++stats.drawCalls;
            stats.triangleCount += item.mesh->indexCount / 3;
        }
    };

    BeginDebugLabel(cmd, "OpaquePass");
    recordItems(opaquePipeline, frameOpaque);
    EndDebugLabel(cmd);

    BeginDebugLabel(cmd, "TransparentPass");
    recordItems(transparentPipeline, frameTransparent);
    EndDebugLabel(cmd);
}

void Renderer::Impl::RecordShadowPass(VkCommandBuffer cmd) {
    const uint32_t cascadeCount = static_cast<uint32_t>(frameScene.params.y + 0.5f);
    if (cascadeCount == 0 || !shadowPipeline.IsValid()) {
        return;
    }
    VulkanTexture& atlas = curTargets->Frame(frameIndex).shadowAtlas;
    const uint32_t atlasRes = curTargets->ShadowResolution();
    const float tileSize = static_cast<float>(atlasRes) * 0.5f;

    TransitionImage(cmd, atlas.image, atlas.currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
    atlas.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    BeginDebugLabel(cmd, "ShadowPass");
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = atlas.view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea = {{0, 0}, {atlasRes, atlasRes}};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 0;
    rendering.pDepthAttachment = &depthAttachment;
    vkCmdBeginRendering(cmd, &rendering);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipeline);
    for (uint32_t c = 0; c < cascadeCount; ++c) {
        VkViewport viewport{};
        viewport.x = static_cast<float>(c & 1u) * tileSize;
        viewport.y = static_cast<float>(c >> 1u) * tileSize;
        viewport.width = tileSize;
        viewport.height = tileSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
                         {static_cast<uint32_t>(tileSize), static_cast<uint32_t>(tileSize)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        for (const DrawItem& item : frameOpaque) {
            const glm::mat4 mvp = frameScene.cascadeViewProj[c] * item.model;
            vkCmdPushConstants(cmd, shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
            const VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &item.mesh->vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, item.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, item.mesh->indexCount, 1, 0, 0, 0);
            ++stats.shadowDrawCalls;
        }
    }
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);

    TransitionImage(cmd, atlas.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    atlas.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void Renderer::Impl::RecordAmbientOcclusion(VkCommandBuffer cmd, VulkanTexture& depth) {
    PostTargets& t = curTargets->Frame(frameIndex);
    VulkanTexture& aoRaw = t.aoRaw;
    VulkanTexture& aoBlur = t.aoBlur;
    const VkExtent2D extent = curTargets->Extent();
    const std::array<float, 4> noClear{0.0f, 0.0f, 0.0f, 1.0f};

    // --- Depth prepass: opaque depth from the camera ------------------------
    TransitionImage(cmd, depth.image, depth.currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
    depth.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    BeginDebugLabel(cmd, "DepthPrepass");
    {
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depth.view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};
        VkRenderingInfo rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering.renderArea = {{0, 0}, extent};
        rendering.layerCount = 1;
        rendering.pDepthAttachment = &depthAttachment;
        vkCmdBeginRendering(cmd, &rendering);
        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPrepassPipeline.pipeline);
        for (const DrawItem& item : frameOpaque) {
            const glm::mat4 mvp = frameViewProj * item.model;
            vkCmdPushConstants(cmd, shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
            const VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &item.mesh->vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, item.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, item.mesh->indexCount, 1, 0, 0, 0);
        }
        vkCmdEndRendering(cmd);
    }
    EndDebugLabel(cmd);

    // Depth -> sampled for the SSAO pass.
    TransitionImage(cmd, depth.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    depth.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- SSAO from depth ----------------------------------------------------
    TransitionImage(cmd, aoRaw.image, aoRaw.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    aoRaw.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    BeginDebugLabel(cmd, "SSAOPass");
    BeginRenderingScope(cmd, extent, aoRaw.view, VK_ATTACHMENT_LOAD_OP_DONT_CARE, noClear, VK_NULL_HANDLE, false);
    SsaoPush ssao;
    ssao.params = glm::vec4(frameNear, frameFar, 0.04f, 1.0f);
    ssao.params2 = glm::vec4(static_cast<float>(AoSampleCount(settings.aoQuality)), 0.05f, 0.0f, 0.0f);
    post.Draw(cmd, frameIndex, post.Ssao(), depth.view, samplers.Nearest(), &ssao, sizeof(ssao));
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    ++stats.postProcessPasses;
    TransitionImage(cmd, aoRaw.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    aoRaw.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- SSAO blur ----------------------------------------------------------
    TransitionImage(cmd, aoBlur.image, aoBlur.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    aoBlur.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    BeginDebugLabel(cmd, "SSAOBlur");
    BeginRenderingScope(cmd, extent, aoBlur.view, VK_ATTACHMENT_LOAD_OP_DONT_CARE, noClear, VK_NULL_HANDLE, false);
    SsaoBlurPush blur;
    blur.texel =
        glm::vec4(1.0f / static_cast<float>(extent.width), 1.0f / static_cast<float>(extent.height), 0.0f, 0.0f);
    post.Draw(cmd, frameIndex, post.SsaoBlur(), aoRaw.view, samplers.LinearClamp(), &blur, sizeof(blur));
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    ++stats.postProcessPasses;
    TransitionImage(cmd, aoBlur.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    aoBlur.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Restore depth to a writable attachment; the main pass loads it.
    TransitionImage(cmd, depth.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    depth.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
}

void Renderer::Impl::RecordBloom(VkCommandBuffer cmd, VulkanTexture& hdr) {
    PostTargets& t = curTargets->Frame(frameIndex);
    const uint32_t mipCount = static_cast<uint32_t>(t.bloomMips.size());
    if (mipCount == 0) {
        return;
    }
    const VkExtent2D extent = curTargets->Extent();
    const VkSampler sampler = samplers.LinearClamp();
    const std::array<float, 4> noClear{0.0f, 0.0f, 0.0f, 1.0f};

    // Threshold-extract + progressive downsample: hdr -> mip0 -> mip1 -> ...
    BeginDebugLabel(cmd, "BloomDownsample");
    for (uint32_t i = 0; i < mipCount; ++i) {
        VulkanTexture& dst = t.bloomMips[i];
        const VkImageView src = (i == 0) ? hdr.view : t.bloomMips[i - 1].view;
        const VkExtent2D srcExtent = (i == 0) ? extent : t.bloomMips[i - 1].extent;
        TransitionImage(cmd, dst.image, dst.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_ASPECT_COLOR_BIT);
        dst.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        BeginRenderingScope(cmd, dst.extent, dst.view, VK_ATTACHMENT_LOAD_OP_DONT_CARE, noClear, VK_NULL_HANDLE, false);
        BloomDownPush push;
        push.texel = glm::vec4(1.0f / static_cast<float>(srcExtent.width), 1.0f / static_cast<float>(srcExtent.height),
                               (i == 0) ? kBloomThreshold : 0.0f, 0.0f);
        post.Draw(cmd, frameIndex, post.BloomDown(), src, sampler, &push, sizeof(push));
        vkCmdEndRendering(cmd);
        TransitionImage(cmd, dst.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        dst.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ++stats.postProcessPasses;
    }
    EndDebugLabel(cmd);

    // Additive upsample back up the chain: mip[i] -> mip[i-1].
    BeginDebugLabel(cmd, "BloomUpsample");
    for (uint32_t i = mipCount - 1; i >= 1; --i) {
        VulkanTexture& src = t.bloomMips[i];
        VulkanTexture& dst = t.bloomMips[i - 1];
        TransitionImage(cmd, dst.image, dst.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_ASPECT_COLOR_BIT);
        dst.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        BeginRenderingScope(cmd, dst.extent, dst.view, VK_ATTACHMENT_LOAD_OP_LOAD, noClear, VK_NULL_HANDLE, false);
        BloomUpPush push;
        push.params = glm::vec4(1.0f / static_cast<float>(src.extent.width), 1.0f, 0.0f, 0.0f);
        post.Draw(cmd, frameIndex, post.BloomUp(), src.view, sampler, &push, sizeof(push));
        vkCmdEndRendering(cmd);
        TransitionImage(cmd, dst.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        dst.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ++stats.postProcessPasses;
    }
    EndDebugLabel(cmd);

    // Composite the brightest mip back into the HDR target, weighted by alpha.
    BeginDebugLabel(cmd, "BloomComposite");
    TransitionImage(cmd, hdr.image, hdr.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    hdr.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    BeginRenderingScope(cmd, extent, hdr.view, VK_ATTACHMENT_LOAD_OP_LOAD, noClear, VK_NULL_HANDLE, false);
    BloomUpPush composite;
    composite.params = glm::vec4(1.0f / static_cast<float>(t.bloomMips[0].extent.width), kBloomIntensity, 0.0f, 0.0f);
    post.Draw(cmd, frameIndex, post.BloomUp(), t.bloomMips[0].view, sampler, &composite, sizeof(composite));
    vkCmdEndRendering(cmd);
    TransitionImage(cmd, hdr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    hdr.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ++stats.postProcessPasses;
    EndDebugLabel(cmd);
}

void Renderer::Impl::RecordDebugLines(VkCommandBuffer cmd, VkImageView color, VkExtent2D extent) {
    if (debugDraw.Empty() || !debugLinePipeline.IsValid()) {
        return;
    }
    const std::vector<DebugDraw::LineVertex>& verts = debugDraw.Vertices();
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(verts.size() * sizeof(DebugDraw::LineVertex));

    // Grow the per-frame line buffer on demand, then upload (CPU-visible). Each
    // frame-in-flight has its own buffer, so writing here never races the GPU
    // (BeginFrame waited on this frame's fence).
    VulkanBuffer& vb = debugVertexBuffers[frameIndex];
    if (!vb.IsValid() || vb.size < bytes) {
        if (vb.IsValid()) {
            DestroyBuffer(device, vb);
        }
        BufferDesc desc;
        desc.size = std::max<VkDeviceSize>(bytes, 4096);
        desc.usage = BufferUsage::Vertex;
        desc.cpuVisible = true;
        desc.debugName = "DebugLineVB";
        vb = CreateBuffer(device, desc);
        if (!vb.IsValid()) {
            return;
        }
    }
    if (vb.mapped != nullptr) {
        std::memcpy(vb.mapped, verts.data(), static_cast<size_t>(bytes));
    }

    BeginDebugLabel(cmd, "DebugLines");
    BeginRenderingScope(cmd, extent, color, VK_ATTACHMENT_LOAD_OP_LOAD, clearColor, VK_NULL_HANDLE, false);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugLinePipeline.pipeline);
    vkCmdPushConstants(cmd, debugLinePipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &frameViewProj);
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb.buffer, &offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, 0, 0);
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
}

void Renderer::Impl::RenderSceneInternal(Scene& scene, const CameraRenderData& camera, VkImageView outputColor,
                                         VkExtent2D outputExtent) {
    FrameData& frame = frames.Frame(frameIndex);
    VkCommandBuffer cmd = frame.commandBuffer;
    PostTargets& t = curTargets->Frame(frameIndex);
    VulkanTexture& hdr = t.hdrColor;
    VulkanTexture& ldr = t.ldrColor;
    VulkanTexture& depth = (*curDepth)[frameIndex];
    const VkExtent2D extent = curTargets->Extent();

    // Gather camera/lights/cascades/renderables, then upload the scene UBO so
    // the cascade matrices are visible to both the shadow and main passes.
    GatherScene(scene, camera);
    if (sceneUBOs[frameIndex].mapped != nullptr) {
        std::memcpy(sceneUBOs[frameIndex].mapped, &frameScene, sizeof(frameScene));
    }

    // --- Shadow pass: render cascades into the depth atlas ------------------
    RecordShadowPass(cmd);

    // --- Ambient occlusion: depth prepass + SSAO (fills the depth target) ---
    const bool aoEnabled = settings.aoQuality != AmbientOcclusionQuality::Off && depth.IsValid();
    if (aoEnabled) {
        RecordAmbientOcclusion(cmd, depth);
    }

    // --- Scene pass: forward PBR into the HDR target ------------------------
    TransitionImage(cmd, hdr.image, hdr.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    hdr.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (!aoEnabled) {
        // No prepass: clear depth here. With AO, depth is already filled + in the
        // DEPTH_ATTACHMENT layout, so the pass loads it instead.
        TransitionImage(cmd, depth.image, depth.currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_ASPECT_DEPTH_BIT);
        depth.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    BeginDebugLabel(cmd, "ScenePass");
    BeginRenderingScope(cmd, extent, hdr.view, VK_ATTACHMENT_LOAD_OP_CLEAR, clearColor, depth.view, !aoEnabled);
    RecordSceneDrawsInPass(cmd);
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);

    TransitionImage(cmd, hdr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    hdr.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- Bloom: extract/downsample/upsample, composited back into HDR -------
    if (settings.bloom) {
        RecordBloom(cmd, hdr);
    }

    // --- Tone map HDR -> LDR (display space) --------------------------------
    TransitionImage(cmd, ldr.image, ldr.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    ldr.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    BeginDebugLabel(cmd, "TonemapPass");
    BeginRenderingScope(cmd, extent, ldr.view, VK_ATTACHMENT_LOAD_OP_DONT_CARE, clearColor, VK_NULL_HANDLE, false);
    TonemapPush tone;
    tone.params = glm::vec4(settings.brightness, ToneMapperMode(settings.toneMapper), 2.2f, 0.0f);
    post.Draw(cmd, frameIndex, post.Tonemap(), hdr.view, samplers.LinearClamp(), &tone, sizeof(tone));
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    ++stats.postProcessPasses;
    TransitionImage(cmd, ldr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    ldr.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- FXAA / present: LDR -> output color --------------------------------
    BeginDebugLabel(cmd, "FXAAPass");
    BeginRenderingScope(cmd, outputExtent, outputColor, VK_ATTACHMENT_LOAD_OP_DONT_CARE, clearColor, VK_NULL_HANDLE,
                        false);
    FxaaPush fxaa;
    fxaa.params = glm::vec4(1.0f / static_cast<float>(extent.width), 1.0f / static_cast<float>(extent.height),
                            settings.antiAliasing == AntiAliasing::FXAA ? 1.0f : 0.0f, 0.0f);
    post.Draw(cmd, frameIndex, post.Fxaa(), ldr.view, samplers.LinearClamp(), &fxaa, sizeof(fxaa));
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    ++stats.postProcessPasses;

    // --- Debug lines overlay: drawn on top of the final image ---------------
    RecordDebugLines(cmd, outputColor, outputExtent);
}

void Renderer::RenderScene(Scene& scene, const CameraRenderData& camera) {
    Impl& r = *m_Impl;
    if (!r.frameActive || !r.targets.IsValid()) {
        return;
    }
    r.curTargets = &r.targets;
    r.curGlobalSets = &r.globalSets;
    r.curDepth = &r.depthTargets;

    VkCommandBuffer cmd = r.frames.Frame(r.frameIndex).commandBuffer;
    // The swapchain image is the FXAA output; transition it to a color target.
    TransitionImage(cmd, r.swapchain.Image(r.imageIndex), r.swapchainLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    r.swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    r.RenderSceneInternal(scene, camera, r.swapchain.ImageView(r.imageIndex), r.swapchain.Extent());
    r.sceneRendered = true;
}

void Renderer::EndFrame() {
    Impl& r = *m_Impl;
    if (!r.frameActive) {
        return;
    }
    FrameData& frame = r.frames.Frame(r.frameIndex);
    VkCommandBuffer cmd = frame.commandBuffer;

    if (!r.sceneRendered) {
        // No scene rendered: clear the swapchain directly so the frame presents.
        TransitionImage(cmd, r.swapchain.Image(r.imageIndex), r.swapchainLayout,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        r.swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        BeginRenderingScope(cmd, r.swapchain.Extent(), r.swapchain.ImageView(r.imageIndex), VK_ATTACHMENT_LOAD_OP_CLEAR,
                            r.clearColor, VK_NULL_HANDLE, false);
        vkCmdEndRendering(cmd);
    }

    TransitionImage(cmd, r.swapchain.Image(r.imageIndex), r.swapchainLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    r.swapchainLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    EndDebugLabel(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphoreSubmitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo.semaphore = frame.imageAvailableSemaphore;
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo.semaphore = r.frames.RenderFinished(r.imageIndex);
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBufferSubmitInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdInfo.commandBuffer = cmd;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &waitInfo;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signalInfo;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;

    vkQueueSubmit2(r.device.graphicsQueue, 1, &submit, frame.inFlightFence);

    VkSemaphore renderFinished = r.frames.RenderFinished(r.imageIndex);
    VkSwapchainKHR swapchain = r.swapchain.Get();
    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &r.imageIndex;

    const VkResult result = vkQueuePresentKHR(r.device.presentQueue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        r.pendingResize = true;
    } else if (result != VK_SUCCESS) {
        HK_CORE_ERROR("vkQueuePresentKHR failed: {}", VkResultString(result));
    }

    // Refresh VRAM stats occasionally (cheap query).
    r.allocator.QueryMemory(r.stats.usedVRAMBytes, r.stats.budgetVRAMBytes);

    r.debugDraw.Clear();
    r.frameIndex = (r.frameIndex + 1) % kFramesInFlight;
    r.frameActive = false;
}

void Renderer::Resize(uint32_t width, uint32_t height) {
    Impl& r = *m_Impl;
    r.pendingResize = true;
    r.pendingWidth = width;
    r.pendingHeight = height;
}

// ---------------------------------------------------------------------------
// Resource / offscreen / built-ins: implemented in later steps.
// ---------------------------------------------------------------------------

Renderer::Impl::OffscreenTarget* Renderer::Impl::ResolveRenderTarget(TextureHandle handle) {
    if (handle.id < kRenderTargetIdBase) {
        return nullptr;
    }
    const uint32_t index = handle.id - kRenderTargetIdBase;
    if (index >= renderTargets.size() || !renderTargets[index].valid) {
        return nullptr;
    }
    return &renderTargets[index];
}

// (Re)builds an offscreen target's intermediates + color/depth images at its
// current size, then refreshes the shadow/AO image bindings of its global sets.
Status Renderer::Impl::BuildOffscreenTarget(OffscreenTarget& t) {
    if (Status s = t.intermediates.Build(t.width, t.height, swapchain.ColorFormat(), settings); !s) {
        return s;
    }
    const TextureFormat colorFormat = SwapchainLogicalFormat(swapchain.ColorFormat());
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        if (t.color[f].IsValid()) {
            DestroyTexture(device, t.color[f]);
        }
        if (t.depth[f].IsValid()) {
            DestroyTexture(device, t.depth[f]);
        }
        TextureDesc colorDesc;
        colorDesc.width = t.width;
        colorDesc.height = t.height;
        colorDesc.format = colorFormat;
        colorDesc.usageFlags = TextureUsage_Sampled | TextureUsage_RenderTarget | TextureUsage_TransferSrc;
        colorDesc.debugName = t.debugName.empty() ? std::string("RenderTargetColor") : t.debugName + ".Color";
        t.color[f] = ::Hockey::CreateTexture(device, command, colorDesc);
        if (!t.color[f].IsValid()) {
            return Status::Fail("Failed to create offscreen color target");
        }
        if (t.hasDepth) {
            TextureDesc depthDesc;
            depthDesc.width = t.width;
            depthDesc.height = t.height;
            depthDesc.format = TextureFormat::Depth32F;
            depthDesc.usageFlags = TextureUsage_DepthStencil | TextureUsage_Sampled;
            depthDesc.debugName = t.debugName.empty() ? std::string("RenderTargetDepth") : t.debugName + ".Depth";
            t.depth[f] = ::Hockey::CreateTexture(device, command, depthDesc);
            if (!t.depth[f].IsValid()) {
                return Status::Fail("Failed to create offscreen depth target");
            }
        }
        WriteGlobalSet(t.globalSets[f], f, t.intermediates);
    }
    return Status::Ok();
}

void Renderer::Impl::DestroyOffscreenTarget(OffscreenTarget& t) {
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        DestroyTexture(device, t.color[f]);
        DestroyTexture(device, t.depth[f]);
    }
    t.intermediates.Destroy();
    t.valid = false;
}

TextureHandle Renderer::CreateRenderTarget(const RenderTargetDesc& desc) {
    Impl& r = *m_Impl;
    if (!r.initialized || r.swapchain.Get() == VK_NULL_HANDLE) {
        return {};
    }
    vkDeviceWaitIdle(r.device.device);

    Impl::OffscreenTarget target;
    target.width = std::max(1u, desc.width);
    target.height = std::max(1u, desc.height);
    target.hasDepth = desc.hasDepth;
    target.debugName = desc.debugName;

    if (Status s = target.intermediates.Create(r.device, r.command); !s) {
        return {};
    }
    // Global sets share the per-frame camera/scene UBOs (bindings 0/1); the
    // shadow/AO images (bindings 2/3) are filled by BuildOffscreenTarget.
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        target.globalSets[f] = r.descriptorAllocator.Allocate(r.descriptorLayouts.global);
        if (target.globalSets[f] == VK_NULL_HANDLE) {
            target.intermediates.Destroy();
            return {};
        }
        DescriptorWriter writer;
        writer.WriteBuffer(0, r.cameraUBOs[f].buffer, r.cameraUBOs[f].size, 0, DescriptorType::UniformBuffer)
            .WriteBuffer(1, r.sceneUBOs[f].buffer, r.sceneUBOs[f].size, 0, DescriptorType::UniformBuffer);
        writer.Update(r.device, target.globalSets[f]);
    }
    if (Status s = r.BuildOffscreenTarget(target); !s) {
        r.DestroyOffscreenTarget(target);
        return {};
    }
    target.valid = true;

    // Reuse a freed slot if one exists, else append.
    uint32_t index = static_cast<uint32_t>(r.renderTargets.size());
    for (uint32_t i = 0; i < r.renderTargets.size(); ++i) {
        if (!r.renderTargets[i].valid) {
            index = i;
            break;
        }
    }
    if (index == r.renderTargets.size()) {
        r.renderTargets.push_back(std::move(target));
    } else {
        r.renderTargets[index] = std::move(target);
    }
    return TextureHandle{kRenderTargetIdBase + index};
}

void Renderer::ResizeRenderTarget(TextureHandle target, uint32_t width, uint32_t height) {
    Impl& r = *m_Impl;
    Impl::OffscreenTarget* t = r.ResolveRenderTarget(target);
    if (t == nullptr) {
        return;
    }
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (t->width == width && t->height == height) {
        return;
    }
    vkDeviceWaitIdle(r.device.device);
    t->width = width;
    t->height = height;
    r.BuildOffscreenTarget(*t);
}

void Renderer::RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target) {
    Impl& r = *m_Impl;
    if (!r.frameActive) {
        return;
    }
    Impl::OffscreenTarget* t = r.ResolveRenderTarget(target);
    if (t == nullptr || !t->intermediates.IsValid() || !t->hasDepth) {
        return; // SSAO/scene depth require a depth buffer on the target
    }
    VkCommandBuffer cmd = r.frames.Frame(r.frameIndex).commandBuffer;
    VulkanTexture& color = t->color[r.frameIndex];

    r.curTargets = &t->intermediates;
    r.curGlobalSets = &t->globalSets;
    r.curDepth = &t->depth;

    TransitionImage(cmd, color.image, color.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    color.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    BeginDebugLabel(cmd, "RenderToTarget");
    r.RenderSceneInternal(scene, camera, color.view, {t->width, t->height});
    EndDebugLabel(cmd);

    // Leave the result sampleable (ImGui later) / transferable (blit now).
    TransitionImage(cmd, color.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    color.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void Renderer::BlitTargetToSwapchain(TextureHandle target) {
    Impl& r = *m_Impl;
    if (!r.frameActive || r.swapchain.Get() == VK_NULL_HANDLE) {
        return;
    }
    Impl::OffscreenTarget* t = r.ResolveRenderTarget(target);
    if (t == nullptr) {
        return;
    }
    VkCommandBuffer cmd = r.frames.Frame(r.frameIndex).commandBuffer;
    VulkanTexture& color = t->color[r.frameIndex];
    VkImage swapImage = r.swapchain.Image(r.imageIndex);
    const VkExtent2D swapExtent = r.swapchain.Extent();

    TransitionImage(cmd, color.image, color.currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    color.currentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    TransitionImage(cmd, swapImage, r.swapchainLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    r.swapchainLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[1] = {static_cast<int32_t>(t->width), static_cast<int32_t>(t->height), 1};
    blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.dstOffsets[1] = {static_cast<int32_t>(swapExtent.width), static_cast<int32_t>(swapExtent.height), 1};
    vkCmdBlitImage(cmd, color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    // Restore the color image so it stays sampleable for ImGui next frame.
    TransitionImage(cmd, color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    color.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    r.sceneRendered = true; // swapchain now has content; skip the clear fallback
}

MeshHandle Renderer::CreateMesh(const MeshDesc& desc) {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return {};
    }
    VulkanMesh mesh = ::Hockey::CreateMesh(r.device, r.command, desc);
    if (!mesh.IsValid()) {
        return {};
    }
    r.meshes.push_back(mesh);
    r.stats.meshCount = static_cast<uint32_t>(r.meshes.size());
    return MeshHandle{static_cast<uint32_t>(r.meshes.size())};
}
TextureHandle Renderer::CreateTexture(const TextureDesc& desc) {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return {};
    }
    VulkanTexture tex = ::Hockey::CreateTexture(r.device, r.command, desc);
    if (!tex.IsValid()) {
        return {};
    }
    r.textures.push_back(tex);
    r.stats.textureCount = static_cast<uint32_t>(r.textures.size());
    return TextureHandle{static_cast<uint32_t>(r.textures.size())};
}
MaterialHandle Renderer::CreateMaterial(const MaterialDesc& desc) {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return {};
    }
    return MaterialHandle{r.CreateMaterialInternal(desc)};
}
MeshHandle Renderer::GetBuiltInMesh(BuiltInMesh mesh) const {
    const auto index = static_cast<size_t>(mesh);
    if (index < m_Impl->builtInMeshes.size()) {
        return m_Impl->builtInMeshes[index];
    }
    return {};
}
MaterialHandle Renderer::GetBuiltInMaterial(BuiltInMaterial material) const {
    const auto index = static_cast<size_t>(material);
    if (index < m_Impl->builtInMaterials.size()) {
        return m_Impl->builtInMaterials[index];
    }
    return {};
}

void Renderer::SetAssetManager(AssetManager* assetManager) {
    m_Impl->assetManager = assetManager;
}
void Renderer::InvalidateAsset(uint64_t assetId) {
    // Drop cached handles so the next resolve re-uploads. The previously created
    // GPU resources stay resident until shutdown; hot-reload is infrequent, so
    // the leak is bounded and avoids destroying a resource that may be in flight.
    m_Impl->assetMeshCache.erase(assetId);
    m_Impl->assetMaterialCache.erase(assetId);
    m_Impl->assetTextureCache.erase(assetId);
}
void Renderer::ClearAssetCache() {
    m_Impl->assetMeshCache.clear();
    m_Impl->assetMaterialCache.clear();
    m_Impl->assetTextureCache.clear();
}

const RendererSettings& Renderer::GetSettings() const {
    return m_Impl->settings;
}

Status Renderer::ApplySettings(const RendererSettings& settings) {
    Impl& r = *m_Impl;
    const bool vsyncChanged = r.settings.vsync != settings.vsync;
    // Changes that resize/relayout the offscreen targets (shadow atlas size,
    // bloom chain length) require a target rebuild before the next frame.
    const bool targetsChanged = r.settings.shadowQuality != settings.shadowQuality ||
                                r.settings.aoQuality != settings.aoQuality || r.settings.preset != settings.preset;
    r.settings = settings;
    if (vsyncChanged) {
        r.pendingResize = true; // present mode change requires swapchain rebuild
    } else if (targetsChanged) {
        r.pendingTargetRebuild = true;
    }
    return Status::Ok();
}

const RendererStats& Renderer::GetStats() const {
    return m_Impl->stats;
}

DebugDraw& Renderer::Debug() {
    return m_Impl->debugDraw;
}

// ---------------------------------------------------------------------------
// Editor-only Dear ImGui integration. RendererImGuiAccess is a friend of
// Renderer, so it can reach the private Impl and its Vulkan handles. This is
// the single isolated seam through which Vulkan leaves the renderer; the editor
// drives ImGui_ImplVulkan with these.
// ---------------------------------------------------------------------------

RendererImGuiVulkanInfo RendererImGuiAccess::VulkanInfo(Renderer& renderer) {
    Renderer::Impl& r = *renderer.m_Impl;
    RendererImGuiVulkanInfo info;
    if (!r.initialized || r.swapchain.Get() == VK_NULL_HANDLE) {
        return info;
    }
    info.instance = r.instance.Get();
    info.physicalDevice = r.device.physicalDevice;
    info.device = r.device.device;
    info.apiVersion = r.apiVersion;
    info.graphicsQueueFamily = r.device.graphicsFamily;
    info.graphicsQueue = r.device.graphicsQueue;
    info.imageCount = r.swapchain.ImageCount();
    info.minImageCount = std::max(2u, r.swapchain.ImageCount());
    info.colorFormat = r.swapchain.ColorFormat();
    info.valid = true;
    return info;
}

RendererSampledImage RendererImGuiAccess::SampledImage(Renderer& renderer, TextureHandle target) {
    Renderer::Impl& r = *renderer.m_Impl;
    RendererSampledImage image;
    Renderer::Impl::OffscreenTarget* t = r.ResolveRenderTarget(target);
    if (t == nullptr) {
        return image;
    }
    const VulkanTexture& color = t->color[r.frameIndex];
    if (!color.IsValid()) {
        return image;
    }
    image.view = color.view;
    image.sampler = r.samplers.LinearClamp();
    image.valid = image.view != VK_NULL_HANDLE && image.sampler != VK_NULL_HANDLE;
    return image;
}

void RendererImGuiAccess::RecordOverlay(Renderer& renderer, const std::function<void(VkCommandBuffer)>& record) {
    Renderer::Impl& r = *renderer.m_Impl;
    if (!r.frameActive || r.swapchain.Get() == VK_NULL_HANDLE) {
        return;
    }
    VkCommandBuffer cmd = r.frames.Frame(r.frameIndex).commandBuffer;
    const VkExtent2D extent = r.swapchain.Extent();

    TransitionImage(cmd, r.swapchain.Image(r.imageIndex), r.swapchainLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    r.swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    BeginDebugLabel(cmd, "ImGuiOverlay");
    BeginRenderingScope(cmd, extent, r.swapchain.ImageView(r.imageIndex), VK_ATTACHMENT_LOAD_OP_CLEAR, r.clearColor,
                        VK_NULL_HANDLE, false);
    if (record) {
        record(cmd);
    }
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    r.sceneRendered = true; // swapchain now has content; skip the clear fallback
}

} // namespace Hockey
