#include "Hockey/Renderer/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <deque>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image_write.h>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/Timer.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/Light.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/RendererImGuiSupport.hpp"
#include "Hockey/Renderer/UIOverlay.hpp"
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

bool IsRgbaReadbackFormat(VkFormat format) {
    return format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_R8G8B8A8_SRGB;
}

bool IsBgraReadbackFormat(VkFormat format) {
    return format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB;
}

Status WriteScreenshotPng(const std::filesystem::path& path, uint32_t width, uint32_t height, VkFormat format,
                          const void* readbackBytes) {
    if (path.empty()) {
        return Status::Fail("Screenshot path is empty");
    }
    if (width == 0 || height == 0 || readbackBytes == nullptr) {
        return Status::Fail("Screenshot readback is empty");
    }
    if (!IsRgbaReadbackFormat(format) && !IsBgraReadbackFormat(format)) {
        return Status::Fail("Screenshot only supports 8-bit RGBA/BGRA render targets");
    }

    const auto* src = static_cast<const unsigned char*>(readbackBytes);
    std::vector<unsigned char> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t i = (static_cast<size_t>(y) * width + x) * 4u;
            if (IsBgraReadbackFormat(format)) {
                rgba[i + 0] = src[i + 2];
                rgba[i + 1] = src[i + 1];
                rgba[i + 2] = src[i + 0];
                rgba[i + 3] = src[i + 3];
            } else {
                rgba[i + 0] = src[i + 0];
                rgba[i + 1] = src[i + 1];
                rgba[i + 2] = src[i + 2];
                rgba[i + 3] = src[i + 3];
            }
        }
    }

    std::error_code ec;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return Status::Fail("Could not create screenshot directory: " + ec.message());
        }
    }
    if (stbi_write_png(path.string().c_str(), static_cast<int>(width), static_cast<int>(height), 4, rgba.data(),
                       static_cast<int>(width * 4u)) == 0) {
        return Status::Fail("Failed to write screenshot PNG: " + path.string());
    }
    return Status::Ok();
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

// Local (spot/point) light shadows share one depth atlas laid out as a square
// grid of perspective tiles. A spot light uses 1 tile; a point light uses 6
// cube-face tiles. Must match HK_MAX_LOCAL_TILES in common.glsl.
constexpr uint32_t kLocalShadowGrid = 4;
constexpr uint32_t kMaxLocalShadowTiles = kLocalShadowGrid * kLocalShadowGrid;

struct SceneGPU {
    glm::vec4 ambient{0.03f, 0.03f, 0.03f, 1.0f};
    glm::vec4 params{0.0f};             // x light count, y cascade count, z 1/atlasRes, w pcf radius
    glm::vec4 cascadeSplits{0.0f};      // view-space far distance of each cascade
    glm::vec4 cascadeTexelSizes{0.0f};  // world-space texel size for each cascade
    glm::mat4 cascadeViewProj[kMaxCascades];
    GpuLight lights[kMaxLights];
    glm::vec4 localShadowParams{0.0f}; // x 1/atlasRes, y pcf radius, z grid dim, w enabled
    glm::vec4 cascadeShadowParams{0.12f, 0.75f, 0.0f, 0.0f};       // x blend scale, y min blend
    glm::vec4 directionalShadowParams{0.75f, 0.003f, 0.03f, 0.0f}; // x normal scale, y min, z max
    glm::vec4 directionalShadowBias{0.00045f, 0.0018f, 0.00045f, 0.003f};
    glm::vec4 contactShadowParams{35.0f, 0.75f, 0.20f, 0.0015f}; // distance, strength, normal scale, min
    glm::vec4 contactShadowBias{0.00012f, 0.0008f, 0.00012f, 0.0012f};
    glm::vec4 localShadowBias{0.0012f, 0.0002f, 0.004f, 0.0f}; // scale, min, max
    glm::mat4 localShadowViewProj[kMaxLocalShadowTiles];
};

struct MaterialGPU {
    glm::vec4 baseColor{1.0f};
    glm::vec4 mrno{0.0f, 0.5f, 1.0f, 1.0f};
    glm::vec4 emissive{0.0f};
    glm::vec4 alpha{0.0f, 0.5f, 0.0f, 0.0f};
    glm::vec4 uvXform{1.0f, 1.0f, 0.0f, 0.0f}; // xy tiling, zw offset
};

struct MeshPush {
    glm::mat4 model{1.0f};
    glm::vec4 flags{1.0f, 0.0f, 0.0f, 0.0f}; // x receives shadows, y contact shadows
};

GpuLight PackLight(const LightRenderData& light) {
    GpuLight out;
    out.positionRange = glm::vec4(light.position, light.range);
    out.direction = glm::vec4(light.direction, static_cast<float>(light.type));
    out.color = glm::vec4(light.color, light.intensity);
    // spot.z is the shadow enable/index slot; -1 means this light casts none.
    out.spot = glm::vec4(light.cosInner, light.cosOuter, -1.0f, 0.0f);
    return out;
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
    glm::vec4 params{0.1f, 1000.0f, 0.5f, 1.0f};       // near, far, world radius, intensity
    glm::vec4 params2{16.0f, 0.025f, 1.0f, 1.0f};      // sampleCount, normal bias, proj00, proj11
};
struct SsaoBlurPush {
    glm::vec4 texel{0.0f}; // xy = 1/size
};

struct ShadowCasterBounds {
    glm::mat4 model{1.0f};
    glm::vec3 localMin{0.0f};
    glm::vec3 localMax{0.0f};
};

template <typename Fn>
void ForEachBoundsCorner(const glm::vec3& min, const glm::vec3& max, Fn&& fn) {
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                fn(glm::vec3(x ? max.x : min.x, y ? max.y : min.y, z ? max.z : min.z));
            }
        }
    }
}

// Directional-light cascade matrices computed from the camera frustum in light
// space. Cascades intentionally overlap so a caster near a split can still
// shadow receivers sampled from the neighboring cascade.
struct CascadeResult {
    std::array<glm::mat4, kMaxCascades> viewProj{};
    std::array<float, kMaxCascades> splitFar{}; // view-space far distance
    std::array<float, kMaxCascades> texelWorldSize{};
    uint32_t count = 0;
};

CascadeResult ComputeCascades(const CameraRenderData& camera, const glm::vec3& lightDir, uint32_t cascadeCount,
                              float shadowDistance, uint32_t atlasResolution,
                              const std::vector<ShadowCasterBounds>& shadowCasters,
                              const RendererSettings& settings) {
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
    // Tile size within the atlas; one cascade uses the full texture, otherwise
    // cascades are packed into a 2x2 atlas.
    const float tileResolution =
        result.count == 1 ? static_cast<float>(atlasResolution) : static_cast<float>(atlasResolution) * 0.5f;

    const float lambda = std::clamp(settings.shadowCascadeSplitLambda, 0.0f, 1.0f);
    float prevSplit = nearClip;
    for (uint32_t c = 0; c < result.count; ++c) {
        const float p = static_cast<float>(c + 1) / static_cast<float>(result.count);
        const float logSplit = nearClip * std::pow(farClip / nearClip, p);
        const float uniformSplit = nearClip + (farClip - nearClip) * p;
        const float splitFar = glm::mix(uniformSplit, logSplit, lambda);

        // Frustum-slice corners: interpolate full-frustum near->far edges.
        // CSM artifacts usually show up when a receiver just after a split uses
        // the next cascade but the caster is still just before the split. Fit a
        // wider slice than the selection range so each cascade contains nearby
        // casters on both sides of its split.
        const float selectionRange = std::max(splitFar - prevSplit, 0.001f);
        const float overlapScale = std::clamp(settings.shadowCascadeOverlapScale, 0.0f, 1.0f);
        const float minOverlap = std::max(settings.shadowCascadeMinOverlap, 0.0f);
        const float maxOverlap =
            std::max(minOverlap, shadowDistance * std::clamp(settings.shadowCascadeMaxOverlapScale, 0.0f, 1.0f));
        const float casterOverlap = glm::clamp(selectionRange * overlapScale, minOverlap, maxOverlap);
        const float fitNear = c == 0 ? nearClip : std::max(nearClip, prevSplit - casterOverlap);
        const float fitFar =
            std::min(farClip, splitFar + std::min(casterOverlap, selectionRange * 0.5f));
        const float fracNear = (fitNear - nearClip) / (farClip - nearClip);
        const float fracFar = (fitFar - nearClip) / (farClip - nearClip);
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
        radius = std::max(std::ceil(radius * 16.0f) / 16.0f, 1.0f);

        constexpr float depthPad = 8.0f;
        const glm::vec3 eye = center - dir * (radius + depthPad);
        glm::mat4 lightView = glm::lookAtRH(eye, center, up);

        glm::vec3 minLS(std::numeric_limits<float>::max());
        glm::vec3 maxLS(std::numeric_limits<float>::lowest());
        for (const glm::vec3& corner : corners) {
            const glm::vec3 lightSpaceCorner = glm::vec3(lightView * glm::vec4(corner, 1.0f));
            minLS = glm::min(minLS, lightSpaceCorner);
            maxLS = glm::max(maxLS, lightSpaceCorner);
        }

        // Keep cascade X/Y fitted to receivers, but derive depth from actual
        // casters that overlap this light-space tile. Using only the camera
        // frustum for near/far clips directional casters before they can write
        // into the map, which shows up as large rectangular missing-shadow
        // regions on broad receivers.
        const float casterXYPad = std::max(radius * 0.05f, 1.0f);
        for (const ShadowCasterBounds& caster : shadowCasters) {
            glm::vec3 casterMinLS(std::numeric_limits<float>::max());
            glm::vec3 casterMaxLS(std::numeric_limits<float>::lowest());
            ForEachBoundsCorner(caster.localMin, caster.localMax, [&](const glm::vec3& localCorner) {
                const glm::vec3 lightSpaceCorner =
                    glm::vec3(lightView * caster.model * glm::vec4(localCorner, 1.0f));
                casterMinLS = glm::min(casterMinLS, lightSpaceCorner);
                casterMaxLS = glm::max(casterMaxLS, lightSpaceCorner);
            });
            const bool overlapsXY = casterMaxLS.x >= minLS.x - casterXYPad &&
                                    casterMinLS.x <= maxLS.x + casterXYPad &&
                                    casterMaxLS.y >= minLS.y - casterXYPad &&
                                    casterMinLS.y <= maxLS.y + casterXYPad;
            if (!overlapsXY) {
                continue;
            }
            minLS.z = std::min(minLS.z, casterMinLS.z);
            maxLS.z = std::max(maxLS.z, casterMaxLS.z);
        }

        // Snap x/y bounds to shadow texels for stable cascades without inflating
        // the z range. Directional acne is very sensitive to wasted depth span.
        const float unitsPerTexelX = std::max((maxLS.x - minLS.x) / std::max(tileResolution, 1.0f), 1e-4f);
        const float unitsPerTexelY = std::max((maxLS.y - minLS.y) / std::max(tileResolution, 1.0f), 1e-4f);
        minLS.x = std::floor(minLS.x / unitsPerTexelX) * unitsPerTexelX;
        minLS.y = std::floor(minLS.y / unitsPerTexelY) * unitsPerTexelY;
        maxLS.x = minLS.x + std::ceil((maxLS.x - minLS.x) / unitsPerTexelX) * unitsPerTexelX;
        maxLS.y = minLS.y + std::ceil((maxLS.y - minLS.y) / unitsPerTexelY) * unitsPerTexelY;
        result.texelWorldSize[c] =
            std::max((maxLS.x - minLS.x) / std::max(tileResolution, 1.0f),
                     (maxLS.y - minLS.y) / std::max(tileResolution, 1.0f));

        const float nearPlane = std::max(0.0f, -maxLS.z - depthPad);
        const float farPlane = std::max(nearPlane + 1.0f, -minLS.z + depthPad);
        glm::mat4 lightProj =
            glm::orthoRH_ZO(minLS.x, maxLS.x, minLS.y, maxLS.y, nearPlane, farPlane);
        result.viewProj[c] = lightProj * lightView;
        result.splitFar[c] = splitFar;
        prevSplit = splitFar;
    }
    return result;
}

// View-projection for a spot light's single shadow tile. A perspective frustum
// looking along the light direction, opened slightly wider than the outer cone
// so the penumbra near the cone edge still has depth data to compare against.
glm::mat4 ComputeSpotShadowMatrix(const LightRenderData& light) {
    glm::vec3 dir = light.direction;
    if (glm::length(dir) < 1e-5f) {
        dir = glm::vec3(0.0f, -1.0f, 0.0f);
    }
    dir = glm::normalize(dir);
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAtRH(light.position, light.position + dir, up);
    const float outerAngle = std::acos(glm::clamp(light.cosOuter, -1.0f, 1.0f));
    const float fov = glm::clamp(2.0f * outerAngle * 1.1f, glm::radians(1.0f), glm::radians(175.0f));
    const float farClip = std::max(light.range, 0.2f);
    // A range-scaled near plane: a tiny fixed near (e.g. 0.05) collapses almost
    // all perspective depth precision onto the near plane, so distant occluders
    // and receivers map to near-identical depths and the shadow vanishes under
    // the comparison bias. Scaling near with range keeps usable precision.
    const float nearClip = glm::clamp(farClip * 0.02f, 0.2f, farClip * 0.5f);
    const glm::mat4 proj = glm::perspectiveRH_ZO(fov, 1.0f, nearClip, farClip);
    return proj * view;
}

// Six 90-degree perspective view-projections (one per cube face) for a point
// light. Face order is +X,-X,+Y,-Y,+Z,-Z to match the axis selection done in
// SampleLocalShadow (common.glsl).
void ComputePointShadowMatrices(const LightRenderData& light, std::array<glm::mat4, 6>& out) {
    const float farClip = std::max(light.range, 0.2f);
    // Range-scaled near plane for usable perspective depth precision (see the
    // note in ComputeSpotShadowMatrix).
    const float nearClip = glm::clamp(farClip * 0.02f, 0.2f, farClip * 0.5f);
    const glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, nearClip, farClip);
    static const glm::vec3 dirs[6] = {
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
    };
    static const glm::vec3 ups[6] = {
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
    };
    for (int f = 0; f < 6; ++f) {
        out[static_cast<size_t>(f)] = proj * glm::lookAtRH(light.position, light.position + dirs[f], ups[f]);
    }
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
    Timer frameCpuTimer; // measures CPU time from BeginFrame to EndFrame
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
        float viewDepth = 0.0f;     // distance from camera, for transparent sort
        bool castsShadows = true;   // skipped by the shadow cascade pass when false
        bool receivesShadows = true;
    };

    // Descriptor system (Step 7). Standard layouts + a growable allocator. A
    // 1x1 default texture stands in for any texture a material omits. White is
    // the identity for the multiplicative slots (base/MR/occlusion/emissive); the
    // normal slot needs a flat tangent-space normal instead.
    VulkanDescriptorLayouts descriptorLayouts;
    VulkanDescriptorAllocator descriptorAllocator;
    VulkanTexture defaultTexture;
    VulkanTexture defaultNormalTexture;

    // Per-frame-in-flight global uniforms + descriptor sets (camera + scene).
    std::array<VulkanBuffer, kFramesInFlight> cameraUBOs;
    std::array<VulkanBuffer, kFramesInFlight> sceneUBOs;
    std::array<VkDescriptorSet, kFramesInFlight> globalSets{};

    // Resource pools. Handle id == index + 1 (0 == invalid).
    // meshes/materials use std::deque because DrawItem caches raw pointers into
    // them while GatherScene resolves entities, and resolving an asset mesh or
    // material can create a new entry mid-gather. std::deque::push_back never
    // invalidates pointers to existing elements (std::vector would reallocate
    // and dangle every previously captured DrawItem pointer).
    std::vector<VulkanTexture> textures;
    std::vector<VulkanBuffer> buffers;
    std::deque<VulkanMesh> meshes;
    std::deque<GpuMaterial> materials;

    struct UIOverlayGeometryResource {
        bool valid = false;
        std::vector<UIOverlayVertex> vertices;
        std::vector<uint32_t> indices;
        std::string debugName;
    };
    struct UIOverlayTextureResource {
        bool valid = false;
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<std::byte> rgba8Pixels;
        std::string debugName;
    };
    std::vector<UIOverlayGeometryResource> uiOverlayGeometries;
    std::vector<UIOverlayTextureResource> uiOverlayTextures;
    std::vector<UIOverlayDrawCommand> uiOverlayCommands;

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
    // Number of local-shadow atlas tiles in use this frame (spot=1, point=6).
    uint32_t frameLocalShadowTiles = 0;
    glm::mat4 frameViewProj{1.0f};
    float frameNear = 0.1f;
    float frameFar = 1000.0f;
    // Projection diagonal terms (signed) so the SSAO pass can reconstruct
    // view-space positions from depth without the full matrix.
    float frameProj00 = 1.0f;
    float frameProj11 = 1.0f;

    // Per-frame-in-flight depth targets, sized to the swapchain and recreated on
    // resize. One per frame avoids a hazard between concurrent in-flight frames.
    std::array<VulkanTexture, kFramesInFlight> depthTargets;

    // The target set the in-progress scene render draws into. These point at the
    // swapchain-sized members by default; RenderSceneToTarget repoints them at an
    // offscreen target so the whole forward+post pipeline shares one code path.
    VulkanFrameTargets* curTargets = nullptr;
    std::array<VkDescriptorSet, kFramesInFlight>* curGlobalSets = nullptr;
    std::array<VulkanBuffer, kFramesInFlight>* curCameraUBOs = nullptr;
    std::array<VulkanBuffer, kFramesInFlight>* curSceneUBOs = nullptr;
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
        std::array<VulkanBuffer, kFramesInFlight> cameraUBOs;
        std::array<VulkanBuffer, kFramesInFlight> sceneUBOs;
        std::array<VkDescriptorSet, kFramesInFlight> globalSets{};
    };
    std::vector<OffscreenTarget> renderTargets;

    // Current layout of the acquired swapchain image, tracked so EndFrame can
    // transition it to PRESENT regardless of whether a scene render, a blit, or
    // the clear fallback last touched it.
    VkImageLayout swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::filesystem::path shaderSourceDir;
    std::filesystem::path shaderBinaryDir;

    enum class ScreenshotSource { Swapchain, RenderTarget };
    struct ScreenshotRequest {
        ScreenshotSource source = ScreenshotSource::Swapchain;
        TextureHandle target{};
        std::filesystem::path path;
    };
    struct ScreenshotReadback {
        VulkanBuffer buffer;
        std::filesystem::path path;
        uint32_t width = 0;
        uint32_t height = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t frameIndex = 0;
    };
    std::vector<ScreenshotRequest> screenshotRequests;
    std::vector<ScreenshotReadback> screenshotReadbacks;

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
    void DestroyMeshPipelines();
    Status SetupDescriptors();
    Status RebuildTargets();
    void UpdateGlobalImageBindings();
    Status CreateGlobalUniforms(std::array<VulkanBuffer, kFramesInFlight>& cameras,
                                std::array<VulkanBuffer, kFramesInFlight>& scenes, const std::string& debugPrefix);
    void DestroyGlobalUniforms(std::array<VulkanBuffer, kFramesInFlight>& cameras,
                               std::array<VulkanBuffer, kFramesInFlight>& scenes);
    Status AllocateGlobalSets(std::array<VkDescriptorSet, kFramesInFlight>& sets,
                              std::array<VulkanBuffer, kFramesInFlight>& cameras,
                              std::array<VulkanBuffer, kFramesInFlight>& scenes);
    uint32_t CreateMaterialInternal(const MaterialDesc& desc);
    VkImageView ResolveTextureView(TextureHandle handle) const;
    MeshHandle ResolveAssetMesh(uint64_t assetId);
    MaterialHandle ResolveAssetMaterial(uint64_t assetId);
    TextureHandle ResolveAssetTexture(uint64_t assetId);
    void PreviewMaterial(uint64_t materialAssetId, const MaterialPreviewDesc& preview);
    void GatherScene(Scene& scene, const CameraRenderData& camera, const SceneRenderFilter* filter);
    void RecordSceneDrawsInPass(VkCommandBuffer cmd);
    void RecordShadowPass(VkCommandBuffer cmd);
    void RecordLocalShadowPass(VkCommandBuffer cmd);
    void RecordAmbientOcclusion(VkCommandBuffer cmd, VulkanTexture& depth);
    void RecordBloom(VkCommandBuffer cmd, VulkanTexture& hdr);
    void RecordDebugLines(VkCommandBuffer cmd, VkImageView color, VkExtent2D extent);
    void RenderSceneInternal(Scene& scene, const CameraRenderData& camera, VkImageView outputColor,
                             VkExtent2D outputExtent, const SceneRenderFilter* filter);
    std::optional<std::filesystem::path> TakeScreenshotRequest(ScreenshotSource source, TextureHandle target);
    void RecordScreenshotCopy(VkCommandBuffer cmd, VkImage image, VkExtent2D extent, VkFormat format,
                              VkImageLayout layout, const std::filesystem::path& path);
    void FlushCompletedScreenshots(uint32_t completedFrameIndex);
    void FlushAllScreenshots();

    Status BuildOffscreenTarget(OffscreenTarget& target);
    void DestroyOffscreenTarget(OffscreenTarget& target);
    void WriteGlobalSet(VkDescriptorSet set, uint32_t frame, VulkanFrameTargets& setTargets);
    OffscreenTarget* ResolveRenderTarget(TextureHandle handle);
};

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
    gpu.uvXform = {desc.tiling.x, desc.tiling.y, desc.offset.x, desc.offset.y};
    UploadBuffer(device, command, mat.ubo, &gpu, sizeof(gpu));

    mat.set = descriptorAllocator.Allocate(descriptorLayouts.material);
    const VkSampler sampler = samplers.Linear();
    DescriptorWriter writer;
    writer.WriteBuffer(0, mat.ubo.buffer, mat.ubo.size, 0, DescriptorType::UniformBuffer)
        .WriteImage(1, ResolveTextureView(desc.baseColorTexture), sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    DescriptorType::CombinedImageSampler)
        .WriteImage(2,
                    desc.normalTexture.IsValid() ? ResolveTextureView(desc.normalTexture) : defaultNormalTexture.view,
                    sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
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

void Renderer::Impl::PreviewMaterial(uint64_t materialAssetId, const MaterialPreviewDesc& preview) {
    // Ensure the GPU material exists and is cached (creates it from the cooked
    // asset on first touch), then mutate it in place.
    const MaterialHandle handle = ResolveAssetMaterial(materialAssetId);
    if (!handle.IsValid() || handle.id > materials.size()) {
        return;
    }
    GpuMaterial& mat = materials[handle.id - 1];

    // Resolve texture slots up front so we can detect binding changes.
    const TextureHandle baseColorTex = ResolveAssetTexture(preview.baseColorTexture);
    const TextureHandle normalTex = ResolveAssetTexture(preview.normalTexture);
    const TextureHandle mrTex = ResolveAssetTexture(preview.metallicRoughnessTexture);
    const TextureHandle occlusionTex = ResolveAssetTexture(preview.occlusionTexture);
    const TextureHandle emissiveTex = ResolveAssetTexture(preview.emissiveTexture);

    const bool texturesChanged = mat.desc.baseColorTexture.id != baseColorTex.id ||
                                 mat.desc.normalTexture.id != normalTex.id ||
                                 mat.desc.metallicRoughnessTexture.id != mrTex.id ||
                                 mat.desc.occlusionTexture.id != occlusionTex.id ||
                                 mat.desc.emissiveTexture.id != emissiveTex.id;

    mat.desc.baseColor = preview.baseColor;
    mat.desc.metallic = preview.metallic;
    mat.desc.roughness = preview.roughness;
    mat.desc.normalStrength = preview.normalStrength;
    mat.desc.occlusionStrength = preview.occlusionStrength;
    mat.desc.emissiveColor = preview.emissiveColor;
    mat.desc.emissiveStrength = preview.emissiveStrength;
    mat.desc.alphaMode = preview.alphaMode;
    mat.desc.alphaCutoff = preview.alphaCutoff;
    mat.desc.tiling = preview.tiling;
    mat.desc.offset = preview.offset;
    mat.desc.baseColorTexture = baseColorTex;
    mat.desc.normalTexture = normalTex;
    mat.desc.metallicRoughnessTexture = mrTex;
    mat.desc.occlusionTexture = occlusionTex;
    mat.desc.emissiveTexture = emissiveTex;

    // Refill the uniform buffer (host-visible; safe to overwrite for an editor
    // preview - a torn read self-corrects on the next frame).
    MaterialGPU gpu;
    gpu.baseColor = mat.desc.baseColor;
    gpu.mrno = {mat.desc.metallic, mat.desc.roughness, mat.desc.normalStrength, mat.desc.occlusionStrength};
    gpu.emissive = glm::vec4(mat.desc.emissiveColor, mat.desc.emissiveStrength);
    const float mode =
        mat.desc.alphaMode == AlphaMode::Opaque ? 0.0f : (mat.desc.alphaMode == AlphaMode::Mask ? 1.0f : 2.0f);
    gpu.alpha = {mode, mat.desc.alphaCutoff, 0.0f, 0.0f};
    gpu.uvXform = {mat.desc.tiling.x, mat.desc.tiling.y, mat.desc.offset.x, mat.desc.offset.y};
    UploadBuffer(device, command, mat.ubo, &gpu, sizeof(gpu));

    // Only rebind images when a texture slot actually changed. We allocate a
    // fresh descriptor set rather than rewriting the existing one (which a prior
    // frame's draw may still reference); the old set leaks until shutdown, which
    // matches InvalidateAsset's bounded-leak policy and only happens on a drop.
    if (texturesChanged) {
        const VkDescriptorSet newSet = descriptorAllocator.Allocate(descriptorLayouts.material);
        const VkSampler sampler = samplers.Linear();
        DescriptorWriter writer;
        writer.WriteBuffer(0, mat.ubo.buffer, mat.ubo.size, 0, DescriptorType::UniformBuffer)
            .WriteImage(1, ResolveTextureView(mat.desc.baseColorTexture), sampler,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
            .WriteImage(2,
                        mat.desc.normalTexture.IsValid() ? ResolveTextureView(mat.desc.normalTexture)
                                                          : defaultNormalTexture.view,
                        sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
            .WriteImage(3, ResolveTextureView(mat.desc.metallicRoughnessTexture), sampler,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
            .WriteImage(4, ResolveTextureView(mat.desc.occlusionTexture), sampler,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler)
            .WriteImage(5, ResolveTextureView(mat.desc.emissiveTexture), sampler,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DescriptorType::CombinedImageSampler);
        writer.Update(device, newSet);
        mat.set = newSet;
    }
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
        desc.tiling = asset.tiling;
        desc.offset = asset.offset;
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

Status Renderer::Impl::CreateGlobalUniforms(std::array<VulkanBuffer, kFramesInFlight>& cameras,
                                            std::array<VulkanBuffer, kFramesInFlight>& scenes,
                                            const std::string& debugPrefix) {
    const std::string prefix = debugPrefix.empty() ? std::string("Global") : debugPrefix;
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        BufferDesc camDesc;
        camDesc.size = sizeof(CameraGPU);
        camDesc.usage = BufferUsage::Uniform;
        camDesc.cpuVisible = true;
        camDesc.debugName = prefix + ".CameraUBO";
        cameras[f] = CreateBuffer(device, camDesc);

        BufferDesc sceneDesc;
        sceneDesc.size = sizeof(SceneGPU);
        sceneDesc.usage = BufferUsage::Uniform;
        sceneDesc.cpuVisible = true;
        sceneDesc.debugName = prefix + ".SceneUBO";
        scenes[f] = CreateBuffer(device, sceneDesc);

        if (!cameras[f].IsValid() || !scenes[f].IsValid()) {
            DestroyGlobalUniforms(cameras, scenes);
            return Status::Fail("Failed to create global uniform buffers");
        }
    }
    return Status::Ok();
}

void Renderer::Impl::DestroyGlobalUniforms(std::array<VulkanBuffer, kFramesInFlight>& cameras,
                                           std::array<VulkanBuffer, kFramesInFlight>& scenes) {
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        DestroyBuffer(device, cameras[f]);
        DestroyBuffer(device, scenes[f]);
    }
}

Status Renderer::Impl::AllocateGlobalSets(std::array<VkDescriptorSet, kFramesInFlight>& sets,
                                          std::array<VulkanBuffer, kFramesInFlight>& cameras,
                                          std::array<VulkanBuffer, kFramesInFlight>& scenes) {
    for (uint32_t f = 0; f < kFramesInFlight; ++f) {
        sets[f] = descriptorAllocator.Allocate(descriptorLayouts.global);
        if (sets[f] == VK_NULL_HANDLE) {
            return Status::Fail("Failed to allocate global descriptor set");
        }

        DescriptorWriter writer;
        writer.WriteBuffer(0, cameras[f].buffer, cameras[f].size, 0, DescriptorType::UniformBuffer)
            .WriteBuffer(1, scenes[f].buffer, scenes[f].size, 0, DescriptorType::UniformBuffer);
        writer.Update(device, sets[f]);
    }
    return Status::Ok();
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

    // 1x1 flat tangent-space normal (0.5, 0.5, 1.0) so materials without a normal
    // map keep their geometric normal. Bytes are R,G,B,A => 0x80,0x80,0xFF,0xFF.
    const uint32_t flatNormalPixel = 0xFFFF8080u;
    texDesc.initialData = &flatNormalPixel;
    texDesc.debugName = "DefaultNormalTexture";
    defaultNormalTexture = ::Hockey::CreateTexture(device, command, texDesc);
    if (!defaultNormalTexture.IsValid()) {
        return Status::Fail("Failed to create default normal texture");
    }

    // Swapchain camera/scene UBOs + descriptor sets. Offscreen editor viewports
    // allocate their own sets so camera-dependent cascades cannot be overwritten
    // by another viewport rendered later in the same frame.
    if (Status s = CreateGlobalUniforms(cameraUBOs, sceneUBOs, "Swapchain"); !s) {
        return s;
    }
    if (Status s = AllocateGlobalSets(globalSets, cameraUBOs, sceneUBOs); !s) {
        return s;
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
    // Binding 4: local (spot/point) shadow atlas (placeholder when shadows off).
    VkImageView localShadowView = setTargets.ShadowPlaceholder().view;
    if (shadowsOn && setTargets.IsValid() && setTargets.Frame(frame).localShadowAtlas.IsValid()) {
        localShadowView = setTargets.Frame(frame).localShadowAtlas.view;
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
    writer.WriteImage(4, localShadowView, samplers.Shadow(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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

void Renderer::Impl::DestroyMeshPipelines() {
    DestroyPipeline(device, opaquePipeline);
    DestroyPipeline(device, transparentPipeline);
    DestroyPipeline(device, shadowPipeline);
    DestroyPipeline(device, depthPrepassPipeline);
    DestroyPipelineLayout(device, meshPipelineLayout);
    meshPipelineLayout = VK_NULL_HANDLE;
    DestroyPipelineLayout(device, shadowPipelineLayout);
    shadowPipelineLayout = VK_NULL_HANDLE;
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
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(MeshPush); // model matrix + per-draw shading flags
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

    // Depth prepass (for SSAO): reuse the mesh vertex shader and layout with no
    // fragment stage so the depth it writes is bit-identical to the main opaque
    // pass. Using a separate transform (e.g. a CPU-computed MVP) would diverge by
    // a few ULPs and reject coplanar fragments, leaking the clear color through.
    GraphicsPipelineDesc prepass = opaque;
    prepass.fragmentShader = VK_NULL_HANDLE; // depth-only
    prepass.colorFormats = {};               // no color attachment
    prepass.depthTest = true;
    prepass.depthWrite = true;
    prepass.blend = BlendMode::Opaque;
    prepass.debugName = "DepthPrepassPipeline";
    depthPrepassPipeline = CreateGraphicsPipeline(device, prepass);

    DestroyShaderModule(device, vs.value.module);
    DestroyShaderModule(device, fs.value.module);

    if (!opaquePipeline.IsValid() || !transparentPipeline.IsValid() || !depthPrepassPipeline.IsValid()) {
        return Status::Fail("Failed to build PBR pipelines");
    }

    // --- Depth-only cascaded-shadow pipeline --------------------------------
    ShaderDesc shadowVsDesc;
    shadowVsDesc.sourcePath = shaderSourceDir / "shadow.vert";
    shadowVsDesc.stage = ShaderStage::Vertex;
    Result<VulkanShaderModule> shadowVs = LoadShaderModule(device, shadowVsDesc, shaderBinaryDir);
    if (!shadowVs) {
        return Status::Fail(shadowVs.error);
    }
    ShaderDesc shadowFsDesc;
    shadowFsDesc.sourcePath = shaderSourceDir / "shadow.frag";
    shadowFsDesc.stage = ShaderStage::Fragment;
    Result<VulkanShaderModule> shadowFs = LoadShaderModule(device, shadowFsDesc, shaderBinaryDir);
    if (!shadowFs) {
        DestroyShaderModule(device, shadowVs.value.module);
        return Status::Fail(shadowFs.error);
    }

    VkPushConstantRange shadowPush{};
    shadowPush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPush.offset = 0;
    shadowPush.size = sizeof(glm::mat4); // light view-proj * model
    shadowPipelineLayout = CreatePipelineLayout(device, {descriptorLayouts.material}, {shadowPush});

    GraphicsPipelineDesc shadow;
    shadow.vertexShader = shadowVs.value.module;
    shadow.layout = shadowPipelineLayout;
    shadow.vertexInput = MeshVertexInput();
    shadow.fragmentShader = shadowFs.value.module;
    shadow.colorFormats = {}; // depth-only
    shadow.depthFormat = depthFormat;
    shadow.topology = PrimitiveTopology::TriangleList;
    // Shadow casters include thin boards and simple built-ins; culling can drop
    // the only useful face when winding or projection orientation varies.
    shadow.cullMode = CullMode::None;
    shadow.depthTest = true;
    shadow.depthWrite = true;
    shadow.depthBias = true;
    shadow.depthBiasConstantFactor = settings.directionalShadowDepthBiasConstant;
    shadow.depthBiasSlopeFactor = settings.directionalShadowDepthBiasSlope;
    shadow.debugName = "ShadowPipeline";
    shadowPipeline = CreateGraphicsPipeline(device, shadow);

    DestroyShaderModule(device, shadowVs.value.module);
    DestroyShaderModule(device, shadowFs.value.module);

    if (shadowPipelineLayout == VK_NULL_HANDLE || !shadowPipeline.IsValid()) {
        return Status::Fail("Failed to build shadow pipeline");
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
    r.settings = ClampRendererSettings(info.settings);
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
        r.FlushAllScreenshots();
    }
}

void Renderer::Shutdown() {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return;
    }
    if (r.device.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(r.device.device);
        r.FlushAllScreenshots();
    }
    r.screenshotRequests.clear();
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
    r.DestroyMeshPipelines();
    DestroyPipeline(r.device, r.debugLinePipeline);
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
    DestroyTexture(r.device, r.defaultNormalTexture);
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
    FlushCompletedScreenshots(frameIndex);

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
    r.frameCpuTimer.Reset();                        // start CPU frame timing
    r.post.BeginFrame(r.frameIndex);               // recycle this frame's transient post sets
    r.swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED; // freshly acquired image

    FrameData& frame = r.frames.Frame(r.frameIndex);
    BeginDebugLabel(frame.commandBuffer, "Frame");
    r.frameActive = true;
}

namespace {

bool AllowsEntity(const SceneRenderFilter* filter, UUID entityId) {
    return filter == nullptr || filter->Allows(entityId);
}

} // namespace

void Renderer::Impl::GatherScene(Scene& scene, const CameraRenderData& camera, const SceneRenderFilter* filter) {
    const VkExtent2D extent = curTargets->Extent();
    frameViewProj = camera.projection * camera.view;
    frameNear = camera.nearClip;
    frameFar = camera.farClip;
    frameProj00 = camera.projection[0][0];
    frameProj11 = camera.projection[1][1];

    // --- Per-frame camera UBO -----------------------------------------------
    CameraGPU cameraGpu;
    cameraGpu.view = camera.view;
    cameraGpu.proj = camera.projection;
    cameraGpu.viewProj = camera.projection * camera.view;
    cameraGpu.position = glm::vec4(camera.position, 1.0f);
    cameraGpu.clip =
        glm::vec4(camera.nearClip, camera.farClip, static_cast<float>(extent.width), static_cast<float>(extent.height));
    if (curCameraUBOs != nullptr) {
        UploadBuffer(device, command, (*curCameraUBOs)[frameIndex], &cameraGpu, sizeof(cameraGpu));
    }

    // --- Lights + environment -----------------------------------------------
    frameScene = SceneGPU{};
    const RendererSettings clampedSettings = ClampRendererSettings(settings);
    frameScene.cascadeShadowParams =
        glm::vec4(clampedSettings.shadowCascadeBlendScale, clampedSettings.shadowCascadeMinBlend, 0.0f, 0.0f);
    frameScene.directionalShadowParams =
        glm::vec4(clampedSettings.directionalShadowNormalOffsetScale,
                  clampedSettings.directionalShadowNormalOffsetMin,
                  clampedSettings.directionalShadowNormalOffsetMax, 0.0f);
    frameScene.directionalShadowBias =
        glm::vec4(clampedSettings.directionalShadowBiasBase, clampedSettings.directionalShadowBiasSlope,
                  clampedSettings.directionalShadowBiasMin, clampedSettings.directionalShadowBiasMax);
    frameScene.contactShadowParams =
        glm::vec4(clampedSettings.contactShadowDistance, clampedSettings.contactShadowStrength,
                  clampedSettings.contactShadowNormalOffsetScale, clampedSettings.contactShadowNormalOffsetMin);
    frameScene.contactShadowBias =
        glm::vec4(clampedSettings.contactShadowBiasBase, clampedSettings.contactShadowBiasSlope,
                  clampedSettings.contactShadowBiasMin, clampedSettings.contactShadowBiasMax);
    frameScene.localShadowBias =
        glm::vec4(clampedSettings.localShadowBiasScale, clampedSettings.localShadowBiasMin,
                  clampedSettings.localShadowBiasMax, 0.0f);
    for (glm::mat4& matrix : frameScene.cascadeViewProj) {
        matrix = glm::mat4(1.0f);
    }
    for (glm::mat4& matrix : frameScene.localShadowViewProj) {
        matrix = glm::mat4(1.0f);
    }
    frameLocalShadowTiles = 0;
    bool hasSun = false;
    uint32_t sunLightIndex = 0;
    glm::vec3 sunDir(0.0f, -1.0f, 0.0f);
    // Shadow-casting spot/point lights, captured so we can assign atlas tiles
    // and build their view-projections after the light loop.
    struct LocalCaster {
        uint32_t lightIndex = 0;
        LightRenderData data;
    };
    std::array<LocalCaster, kMaxLights> localCasters{};
    uint32_t localCasterCount = 0;
    const uint32_t maxRenderedLights = std::min(clampedSettings.maxRenderedLights, kMaxLights);
    const uint32_t maxLocalShadowTiles = std::min(clampedSettings.maxLocalShadowTiles, kMaxLocalShadowTiles);
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
            if (lightCount >= maxRenderedLights) {
                break;
            }
            const Entity entity{handle, &scene};
            if (!AllowsEntity(filter, entity.GetUUID())) {
                continue;
            }
            if (!entity.IsActiveInHierarchy()) {
                continue;
            }
            const auto& light = lightView.get<LightComponent>(handle);
            const glm::mat4 world = scene.GetWorldTransform(entity);
            const LightRenderData data = BuildLightRenderData(world, light);
            frameScene.lights[lightCount] = PackLight(data);
            if (!hasSun && data.type == 0 && data.castsShadows) {
                hasSun = true;
                sunLightIndex = lightCount;
                sunDir = data.direction;
            }
            // Queue spot (type 2) / point (type 1) shadow casters for tiling.
            if (data.castsShadows && (data.type == 1 || data.type == 2) && localCasterCount < kMaxLights) {
                localCasters[localCasterCount] = {lightCount, data};
                ++localCasterCount;
            }
            ++lightCount;
        }
        frameScene.params = glm::vec4(static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f);
    }

    // --- Renderables, resolving names to handles + fallbacks ----------------
    frameOpaque.clear();
    frameTransparent.clear();
    std::vector<ShadowCasterBounds> shadowCasters;
    const auto meshView = scene.Registry().view<TransformComponent, MeshRendererComponent>();
    for (const entt::entity handle : meshView) {
        const Entity entity{handle, &scene};
        if (!AllowsEntity(filter, entity.GetUUID())) {
            continue;
        }
        if (!entity.IsActiveInHierarchy()) {
            continue;
        }
        const auto& mr = meshView.get<MeshRendererComponent>(handle);
        if (!mr.visible) {
            continue;
        }

        MeshHandle meshHandle = ResolveAssetMesh(mr.meshAsset);
        if (!meshHandle.IsValid()) {
            continue;
        }
        MaterialHandle materialHandle = ResolveAssetMaterial(mr.materialAsset);
        if (!materialHandle.IsValid()) {
            continue;
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
        item.castsShadows = mr.castsShadows;
        item.receivesShadows = mr.receivesShadows;
        if (!item.mesh->IsValid()) {
            continue;
        }
        if (item.castsShadows) {
            shadowCasters.push_back({item.model, item.mesh->boundsMin, item.mesh->boundsMax});
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

    // --- Directional cascaded shadows ---------------------------------------
    const uint32_t cascadeCount = ResolveShadowCascadeCount(clampedSettings);
    if (clampedSettings.shadowQuality != ShadowQuality::Off && cascadeCount > 0 && hasSun && curTargets->IsValid()) {
        const uint32_t atlasRes = curTargets->ShadowResolution();
        const CascadeResult cascades = ComputeCascades(camera, sunDir, cascadeCount, clampedSettings.shadowDistance,
                                                       atlasRes, shadowCasters, clampedSettings);
        for (uint32_t c = 0; c < cascades.count; ++c) {
            frameScene.cascadeViewProj[c] = cascades.viewProj[c];
            frameScene.cascadeSplits[c] = cascades.splitFar[c];
            frameScene.cascadeTexelSizes[c] = cascades.texelWorldSize[c];
        }
        const float pcfRadius = static_cast<float>(ResolveDirectionalShadowPcfRadius(clampedSettings));
        frameScene.params.y = static_cast<float>(cascades.count);
        frameScene.params.z = 1.0f / static_cast<float>(std::max(atlasRes, 1u));
        frameScene.params.w = pcfRadius;
        frameScene.lights[sunLightIndex].spot.z = 0.0f;
    }

    // --- Local (spot/point) light shadows -----------------------------------
    // Pack each shadow-casting spot light into one atlas tile and each point
    // light into six cube-face tiles, greedily until the atlas is full.
    const uint32_t localAtlasRes = curTargets->LocalShadowResolution();
    if (clampedSettings.shadowQuality != ShadowQuality::Off && localCasterCount > 0 && curTargets->IsValid() &&
        localAtlasRes > 1) {
        uint32_t tileCursor = 0;
        for (uint32_t i = 0; i < localCasterCount; ++i) {
            const LightRenderData& data = localCasters[i].data;
            const uint32_t needed = data.type == 1 ? 6u : 1u;
            if (tileCursor + needed > maxLocalShadowTiles) {
                break; // atlas full; remaining casters fall back to no shadow
            }
            if (data.type == 2) {
                frameScene.localShadowViewProj[tileCursor] = ComputeSpotShadowMatrix(data);
            } else {
                std::array<glm::mat4, 6> faces;
                ComputePointShadowMatrices(data, faces);
                for (uint32_t f = 0; f < 6; ++f) {
                    frameScene.localShadowViewProj[tileCursor + f] = faces[f];
                }
            }
            frameScene.lights[localCasters[i].lightIndex].spot.z = static_cast<float>(tileCursor);
            tileCursor += needed;
        }
        frameLocalShadowTiles = tileCursor;
        if (tileCursor > 0) {
            const float pcfRadius = static_cast<float>(ResolveLocalShadowPcfRadius(clampedSettings));
            frameScene.localShadowParams = glm::vec4(1.0f / static_cast<float>(localAtlasRes), pcfRadius,
                                                     static_cast<float>(kLocalShadowGrid), 1.0f);
        }
    }
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
            MeshPush push;
            push.model = item.model;
            push.flags = glm::vec4(item.receivesShadows ? 1.0f : 0.0f, settings.contactShadows ? 1.0f : 0.0f, 0.0f,
                                   0.0f);
            vkCmdPushConstants(cmd, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(MeshPush), &push);

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
    const float tileSize = cascadeCount == 1 ? static_cast<float>(atlasRes) : static_cast<float>(atlasRes) * 0.5f;

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
        viewport.x = cascadeCount == 1 ? 0.0f : static_cast<float>(c & 1u) * tileSize;
        viewport.y = cascadeCount == 1 ? 0.0f : static_cast<float>(c >> 1u) * tileSize;
        viewport.width = tileSize;
        viewport.height = tileSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
                         {static_cast<uint32_t>(tileSize), static_cast<uint32_t>(tileSize)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        for (const DrawItem& item : frameOpaque) {
            if (!item.castsShadows) {
                continue; // MeshRendererComponent.castsShadows == false
            }
            const glm::mat4 mvp = frameScene.cascadeViewProj[c] * item.model;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1,
                                    &item.material->set, 0, nullptr);
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

// Renders depth for every active local-shadow tile (spot frusta + point cube
// faces) into the shared local atlas. Mirrors RecordShadowPass but iterates the
// grid tiles assigned in GatherScene instead of directional cascades.
void Renderer::Impl::RecordLocalShadowPass(VkCommandBuffer cmd) {
    if (frameLocalShadowTiles == 0 || !shadowPipeline.IsValid()) {
        return;
    }
    VulkanTexture& atlas = curTargets->Frame(frameIndex).localShadowAtlas;
    if (!atlas.IsValid()) {
        return;
    }
    const uint32_t atlasRes = curTargets->LocalShadowResolution();
    const float tileSize = static_cast<float>(atlasRes) / static_cast<float>(kLocalShadowGrid);

    TransitionImage(cmd, atlas.image, atlas.currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
    atlas.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    BeginDebugLabel(cmd, "LocalShadowPass");
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
    for (uint32_t tile = 0; tile < frameLocalShadowTiles; ++tile) {
        VkViewport viewport{};
        viewport.x = static_cast<float>(tile % kLocalShadowGrid) * tileSize;
        viewport.y = static_cast<float>(tile / kLocalShadowGrid) * tileSize;
        viewport.width = tileSize;
        viewport.height = tileSize;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
                         {static_cast<uint32_t>(tileSize), static_cast<uint32_t>(tileSize)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        const glm::mat4& tileViewProj = frameScene.localShadowViewProj[tile];
        for (const DrawItem& item : frameOpaque) {
            if (!item.castsShadows) {
                continue;
            }
            const glm::mat4 mvp = tileViewProj * item.model;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1,
                                    &item.material->set, 0, nullptr);
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
        // Same transform path as the main opaque pass (mesh.vert + camera UBO +
        // pushed model) so the written depth matches it exactly.
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, 0, 1,
                                &(*curGlobalSets)[frameIndex], 0, nullptr);
        for (const DrawItem& item : frameOpaque) {
            MeshPush push;
            push.model = item.model;
            push.flags = glm::vec4(item.receivesShadows ? 1.0f : 0.0f, settings.contactShadows ? 1.0f : 0.0f, 0.0f,
                                   0.0f);
            vkCmdPushConstants(cmd, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(MeshPush), &push);
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
    // params:  near, far, world-space sample radius, intensity
    // params2: sample count, normal bias, proj[0][0], proj[1][1]
    ssao.params = glm::vec4(frameNear, frameFar, 0.3f, 1.0f);
    ssao.params2 =
        glm::vec4(static_cast<float>(AoSampleCount(settings.aoQuality)), 0.03f, frameProj00, frameProj11);
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
    UploadBuffer(device, command, vb, verts.data(), static_cast<size_t>(bytes));

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

std::optional<std::filesystem::path> Renderer::Impl::TakeScreenshotRequest(ScreenshotSource source,
                                                                           TextureHandle target) {
    for (auto it = screenshotRequests.begin(); it != screenshotRequests.end(); ++it) {
        if (it->source == source && it->target.id == target.id) {
            std::filesystem::path path = it->path;
            screenshotRequests.erase(it);
            return path;
        }
    }
    return std::nullopt;
}

void Renderer::Impl::RecordScreenshotCopy(VkCommandBuffer cmd, VkImage image, VkExtent2D extent, VkFormat format,
                                          VkImageLayout layout, const std::filesystem::path& path) {
    if (image == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        HK_CORE_WARN("Screenshot skipped: invalid image or extent");
        return;
    }
    if (!IsRgbaReadbackFormat(format) && !IsBgraReadbackFormat(format)) {
        HK_CORE_WARN("Screenshot skipped: unsupported format {}", static_cast<int>(format));
        return;
    }

    ScreenshotReadback capture;
    capture.path = path;
    capture.width = extent.width;
    capture.height = extent.height;
    capture.format = format;
    capture.frameIndex = frameIndex;

    BufferDesc readbackDesc;
    readbackDesc.size = static_cast<size_t>(extent.width) * static_cast<size_t>(extent.height) * 4u;
    readbackDesc.usage = BufferUsage::Readback;
    readbackDesc.cpuVisible = true;
    readbackDesc.debugName = "ScreenshotReadback";
    capture.buffer = CreateBuffer(device, readbackDesc);
    if (!capture.buffer.IsValid()) {
        HK_CORE_WARN("Screenshot skipped: failed to allocate readback buffer");
        return;
    }

    TransitionImage(cmd, image, layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, capture.buffer.buffer, 1, &region);
    TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT);

    screenshotReadbacks.push_back(std::move(capture));
}

void Renderer::Impl::FlushCompletedScreenshots(uint32_t completedFrameIndex) {
    for (auto it = screenshotReadbacks.begin(); it != screenshotReadbacks.end();) {
        if (it->frameIndex != completedFrameIndex) {
            ++it;
            continue;
        }
        if (it->buffer.IsValid()) {
            vmaInvalidateAllocation(device.allocator, it->buffer.allocation, 0, it->buffer.size);
        }
        const Status written =
            WriteScreenshotPng(it->path, it->width, it->height, it->format, it->buffer.mapped);
        if (written) {
            HK_CORE_INFO("Screenshot saved: {}", it->path.string());
        } else {
            HK_CORE_WARN("Screenshot failed: {}", written.error);
        }
        DestroyBuffer(device, it->buffer);
        it = screenshotReadbacks.erase(it);
    }
}

void Renderer::Impl::FlushAllScreenshots() {
    for (auto& capture : screenshotReadbacks) {
        if (capture.buffer.IsValid()) {
            vmaInvalidateAllocation(device.allocator, capture.buffer.allocation, 0, capture.buffer.size);
        }
        const Status written =
            WriteScreenshotPng(capture.path, capture.width, capture.height, capture.format, capture.buffer.mapped);
        if (written) {
            HK_CORE_INFO("Screenshot saved: {}", capture.path.string());
        } else {
            HK_CORE_WARN("Screenshot failed: {}", written.error);
        }
        DestroyBuffer(device, capture.buffer);
    }
    screenshotReadbacks.clear();
}

void Renderer::Impl::RenderSceneInternal(Scene& scene, const CameraRenderData& camera, VkImageView outputColor,
                                         VkExtent2D outputExtent, const SceneRenderFilter* filter) {
    FrameData& frame = frames.Frame(frameIndex);
    VkCommandBuffer cmd = frame.commandBuffer;
    PostTargets& t = curTargets->Frame(frameIndex);
    VulkanTexture& hdr = t.hdrColor;
    VulkanTexture& ldr = t.ldrColor;
    VulkanTexture& depth = (*curDepth)[frameIndex];
    const VkExtent2D extent = curTargets->Extent();

    // Gather camera/lights/cascades/renderables, then upload the scene UBO so
    // the cascade matrices are visible to both the shadow and main passes.
    GatherScene(scene, camera, filter);
    if (curSceneUBOs != nullptr) {
        UploadBuffer(device, command, (*curSceneUBOs)[frameIndex], &frameScene, sizeof(frameScene));
    }

    // --- Shadow pass: render cascades into the depth atlas ------------------
    RecordShadowPass(cmd);
    // --- Local shadow pass: spot/point depth into the local atlas -----------
    RecordLocalShadowPass(cmd);

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
    // FXAA is currently the only implemented AA technique. TAA and MSAA* are
    // declared settings but not yet implemented, so they fall back to FXAA
    // rather than silently dropping anti-aliasing (which would make higher
    // presets look worse than lower ones). Only an explicit "Off" disables AA.
    const float fxaaEnabled = settings.antiAliasing != AntiAliasing::Off ? 1.0f : 0.0f;
    fxaa.params = glm::vec4(1.0f / static_cast<float>(extent.width), 1.0f / static_cast<float>(extent.height),
                            fxaaEnabled, 0.0f);
    post.Draw(cmd, frameIndex, post.Fxaa(), ldr.view, samplers.LinearClamp(), &fxaa, sizeof(fxaa));
    vkCmdEndRendering(cmd);
    EndDebugLabel(cmd);
    ++stats.postProcessPasses;

    // --- Debug lines overlay: drawn on top of the final image ---------------
    RecordDebugLines(cmd, outputColor, outputExtent);
}

void Renderer::RenderScene(Scene& scene, const CameraRenderData& camera, const SceneRenderFilter* filter) {
    Impl& r = *m_Impl;
    if (!r.frameActive || !r.targets.IsValid()) {
        return;
    }
    r.curTargets = &r.targets;
    r.curGlobalSets = &r.globalSets;
    r.curCameraUBOs = &r.cameraUBOs;
    r.curSceneUBOs = &r.sceneUBOs;
    r.curDepth = &r.depthTargets;

    VkCommandBuffer cmd = r.frames.Frame(r.frameIndex).commandBuffer;
    // The swapchain image is the FXAA output; transition it to a color target.
    TransitionImage(cmd, r.swapchain.Image(r.imageIndex), r.swapchainLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    r.swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    r.RenderSceneInternal(scene, camera, r.swapchain.ImageView(r.imageIndex), r.swapchain.Extent(), filter);
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

    if (std::optional<std::filesystem::path> path =
            r.TakeScreenshotRequest(Impl::ScreenshotSource::Swapchain, TextureHandle{})) {
        if (r.swapchain.SupportsTransferSrc()) {
            r.RecordScreenshotCopy(cmd, r.swapchain.Image(r.imageIndex), r.swapchain.Extent(),
                                   r.swapchain.ColorFormat(), r.swapchainLayout, *path);
        } else {
            HK_CORE_WARN("Screenshot skipped: swapchain no longer supports transfer-src images");
        }
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
    // CPU time spent producing this frame (record/submit/present). GPU timing
    // (gpuFrameMs) still requires timestamp queries and is left unpopulated.
    r.stats.cpuFrameMs = static_cast<float>(r.frameCpuTimer.ElapsedMilliseconds());
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
    DestroyGlobalUniforms(t.cameraUBOs, t.sceneUBOs);
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
    if (Status s = r.CreateGlobalUniforms(target.cameraUBOs, target.sceneUBOs, target.debugName); !s) {
        target.intermediates.Destroy();
        return {};
    }
    if (Status s = r.AllocateGlobalSets(target.globalSets, target.cameraUBOs, target.sceneUBOs); !s) {
        r.DestroyOffscreenTarget(target);
        return {};
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

void Renderer::RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target,
                                   const SceneRenderFilter* filter) {
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
    r.curCameraUBOs = &t->cameraUBOs;
    r.curSceneUBOs = &t->sceneUBOs;
    r.curDepth = &t->depth;

    TransitionImage(cmd, color.image, color.currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT);
    color.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    BeginDebugLabel(cmd, "RenderToTarget");
    r.RenderSceneInternal(scene, camera, color.view, {t->width, t->height}, filter);
    EndDebugLabel(cmd);

    if (std::optional<std::filesystem::path> path =
            r.TakeScreenshotRequest(Impl::ScreenshotSource::RenderTarget, target)) {
        r.RecordScreenshotCopy(cmd, color.image, {t->width, t->height}, color.format,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, *path);
    }

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

Status Renderer::RequestScreenshot(const std::filesystem::path& path) {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return Status::Fail("Renderer is not initialized");
    }
    if (path.empty()) {
        return Status::Fail("Screenshot path is empty");
    }
    if (r.swapchain.Get() == VK_NULL_HANDLE || !r.swapchain.SupportsTransferSrc()) {
        return Status::Fail("Swapchain screenshots are not supported on this surface");
    }
    r.screenshotRequests.push_back({Impl::ScreenshotSource::Swapchain, TextureHandle{}, path});
    return Status::Ok();
}

Status Renderer::RequestRenderTargetScreenshot(TextureHandle target, const std::filesystem::path& path) {
    Impl& r = *m_Impl;
    if (!r.initialized) {
        return Status::Fail("Renderer is not initialized");
    }
    if (path.empty()) {
        return Status::Fail("Screenshot path is empty");
    }
    if (r.ResolveRenderTarget(target) == nullptr) {
        return Status::Fail("Screenshot target is invalid");
    }
    r.screenshotRequests.push_back({Impl::ScreenshotSource::RenderTarget, target, path});
    return Status::Ok();
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

UIOverlayGeometryHandle Renderer::CreateUIOverlayGeometry(const UIOverlayGeometryDesc& desc) {
    Impl& r = *m_Impl;
    Impl::UIOverlayGeometryResource resource;
    resource.valid = !desc.vertices.empty() && !desc.indices.empty();
    resource.vertices = desc.vertices;
    resource.indices = desc.indices;
    resource.debugName = desc.debugName;
    r.uiOverlayGeometries.push_back(std::move(resource));
    return UIOverlayGeometryHandle{static_cast<uint32_t>(r.uiOverlayGeometries.size())};
}

void Renderer::ReleaseUIOverlayGeometry(UIOverlayGeometryHandle handle) {
    Impl& r = *m_Impl;
    if (!handle.IsValid() || handle.id > r.uiOverlayGeometries.size()) {
        return;
    }
    r.uiOverlayGeometries[handle.id - 1] = {};
}

UIOverlayTextureHandle Renderer::CreateUIOverlayTexture(const UIOverlayTextureDesc& desc) {
    Impl& r = *m_Impl;
    Impl::UIOverlayTextureResource resource;
    resource.valid = desc.width > 0 && desc.height > 0;
    resource.width = desc.width;
    resource.height = desc.height;
    resource.debugName = desc.debugName;
    if (desc.rgba8Pixels != nullptr && desc.rgba8ByteCount > 0) {
        const auto* begin = static_cast<const std::byte*>(desc.rgba8Pixels);
        resource.rgba8Pixels.assign(begin, begin + desc.rgba8ByteCount);
    }
    r.uiOverlayTextures.push_back(std::move(resource));
    return UIOverlayTextureHandle{static_cast<uint32_t>(r.uiOverlayTextures.size())};
}

void Renderer::ReleaseUIOverlayTexture(UIOverlayTextureHandle handle) {
    Impl& r = *m_Impl;
    if (!handle.IsValid() || handle.id > r.uiOverlayTextures.size()) {
        return;
    }
    r.uiOverlayTextures[handle.id - 1] = {};
}

void Renderer::RenderUIOverlay(const std::vector<UIOverlayDrawCommand>& commands) {
    Impl& r = *m_Impl;
    r.uiOverlayCommands.clear();
    r.uiOverlayCommands.reserve(commands.size());
    for (const UIOverlayDrawCommand& command : commands) {
        const bool geometryValid =
            command.geometry.IsValid() && command.geometry.id <= r.uiOverlayGeometries.size() &&
            r.uiOverlayGeometries[command.geometry.id - 1].valid;
        const bool textureValid =
            !command.texture.IsValid() ||
            (command.texture.id <= r.uiOverlayTextures.size() && r.uiOverlayTextures[command.texture.id - 1].valid);
        if (geometryValid && textureValid) {
            r.uiOverlayCommands.push_back(command);
        }
    }
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
void Renderer::PreviewMaterial(uint64_t materialAssetId, const MaterialPreviewDesc& preview) {
    m_Impl->PreviewMaterial(materialAssetId, preview);
}

const RendererSettings& Renderer::GetSettings() const {
    return m_Impl->settings;
}

Status Renderer::ApplySettings(const RendererSettings& settings) {
    Impl& r = *m_Impl;
    const RendererSettings currentSettings = ClampRendererSettings(r.settings);
    const RendererSettings nextSettings = ClampRendererSettings(settings);
    const bool vsyncChanged = currentSettings.vsync != nextSettings.vsync;
    // Changes that resize/relayout the offscreen targets (shadow atlas size,
    // bloom chain length) require a target rebuild before the next frame.
    const bool targetsChanged =
        currentSettings.shadowQuality != nextSettings.shadowQuality ||
        currentSettings.aoQuality != nextSettings.aoQuality || currentSettings.preset != nextSettings.preset ||
        ResolveDirectionalShadowAtlasResolution(currentSettings) !=
            ResolveDirectionalShadowAtlasResolution(nextSettings) ||
        ResolveLocalShadowAtlasResolution(currentSettings) != ResolveLocalShadowAtlasResolution(nextSettings) ||
        ResolveShadowCascadeCount(currentSettings) != ResolveShadowCascadeCount(nextSettings);
    const bool shadowPipelineChanged =
        currentSettings.directionalShadowDepthBiasConstant != nextSettings.directionalShadowDepthBiasConstant ||
        currentSettings.directionalShadowDepthBiasSlope != nextSettings.directionalShadowDepthBiasSlope;
    r.settings = nextSettings;
    if (vsyncChanged) {
        r.pendingResize = true; // present mode change requires swapchain rebuild
    } else if (targetsChanged) {
        r.pendingTargetRebuild = true;
    }
    if (shadowPipelineChanged && r.initialized && r.device.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(r.device.device);
        r.DestroyMeshPipelines();
        return r.BuildMeshPipelines();
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

RendererSampledImage RendererImGuiAccess::SampledImageForAsset(Renderer& renderer, uint64_t textureAssetId) {
    Renderer::Impl& r = *renderer.m_Impl;
    RendererSampledImage image;
    const TextureHandle handle = r.ResolveAssetTexture(textureAssetId);
    if (!handle.IsValid() || handle.id > r.textures.size()) {
        return image;
    }
    const VulkanTexture& tex = r.textures[handle.id - 1];
    if (!tex.IsValid()) {
        return image;
    }
    image.view = tex.view;
    image.sampler = r.samplers.Linear();
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
