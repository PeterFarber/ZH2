#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"

#include <cstdint>

#include <imgui.h>

#include <imgui_impl_vulkan.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/Renderer/RendererImGuiSupport.hpp"

namespace Hockey {

namespace {
std::uint64_t HandleToU64(VkImageView view) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(view));
}
std::uint64_t HandleToU64(VkDescriptorSet set) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(set));
}
VkDescriptorSet U64ToDescriptorSet(std::uint64_t value) {
    return reinterpret_cast<VkDescriptorSet>(static_cast<std::uintptr_t>(value));
}
} // namespace

Status ImGuiRendererBridge::Init(Renderer& renderer) {
    if (m_Initialized) {
        return Status::Ok();
    }
    const RendererImGuiVulkanInfo vk = RendererImGuiAccess::VulkanInfo(renderer);
    if (!vk.valid) {
        return Status::Fail("ImGuiRendererBridge::Init requires an initialized renderer with a swapchain");
    }

    VkFormat colorFormat = vk.colorFormat;
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    ImGui_ImplVulkan_InitInfo init{};
    init.ApiVersion = vk.apiVersion;
    init.Instance = vk.instance;
    init.PhysicalDevice = vk.physicalDevice;
    init.Device = vk.device;
    init.QueueFamily = vk.graphicsQueueFamily;
    init.Queue = vk.graphicsQueue;
    init.DescriptorPool = VK_NULL_HANDLE;
    init.DescriptorPoolSize = 64; // backend creates its own pool (fonts + viewport images)
    init.MinImageCount = vk.minImageCount;
    init.ImageCount = vk.imageCount;
    init.UseDynamicRendering = true;
    init.PipelineInfoMain.PipelineRenderingCreateInfo = renderingInfo;

    if (!ImGui_ImplVulkan_Init(&init)) {
        return Status::Fail("ImGui_ImplVulkan_Init failed");
    }

    m_Renderer = &renderer;
    m_Initialized = true;
    HK_EDITOR_INFO("ImGui Vulkan backend initialized");
    return Status::Ok();
}

void ImGuiRendererBridge::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    InvalidateViewportTextures();
    for (const auto& [view, set] : m_AssetTextures) {
        (void)view;
        ImGui_ImplVulkan_RemoveTexture(U64ToDescriptorSet(set));
    }
    m_AssetTextures.clear();
    ImGui_ImplVulkan_Shutdown();
    m_Initialized = false;
    m_Renderer = nullptr;
}

void ImGuiRendererBridge::NewFrame() {
    if (!m_Initialized) {
        return;
    }
    ImGui_ImplVulkan_NewFrame();
}

void ImGuiRendererBridge::RenderDrawData(ImDrawData* drawData) {
    if (!m_Initialized || m_Renderer == nullptr || drawData == nullptr) {
        return;
    }
    RendererImGuiAccess::RecordOverlay(
        *m_Renderer, [drawData](VkCommandBuffer cmd) { ImGui_ImplVulkan_RenderDrawData(drawData, cmd); });
}

std::uint64_t ImGuiRendererBridge::ViewportTextureId(TextureHandle target) {
    if (!m_Initialized || m_Renderer == nullptr) {
        return 0;
    }
    const RendererSampledImage image = RendererImGuiAccess::SampledImage(*m_Renderer, target);
    if (!image.valid) {
        return 0;
    }
    const std::uint64_t viewKey = HandleToU64(image.view);
    if (const auto it = m_ViewportTextures.find(viewKey); it != m_ViewportTextures.end()) {
        return it->second;
    }
    const VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    const std::uint64_t id = HandleToU64(set);
    m_ViewportTextures.emplace(viewKey, id);
    return id;
}

std::uint64_t ImGuiRendererBridge::TextureIdForAsset(std::uint64_t textureAssetId) {
    if (!m_Initialized || m_Renderer == nullptr || textureAssetId == 0) {
        return 0;
    }
    const RendererSampledImage image = RendererImGuiAccess::SampledImageForAsset(*m_Renderer, textureAssetId);
    if (!image.valid) {
        return 0;
    }
    const std::uint64_t viewKey = HandleToU64(image.view);
    if (const auto it = m_AssetTextures.find(viewKey); it != m_AssetTextures.end()) {
        return it->second;
    }
    const VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    const std::uint64_t id = HandleToU64(set);
    m_AssetTextures.emplace(viewKey, id);
    return id;
}

void ImGuiRendererBridge::InvalidateViewportTextures() {
    for (const auto& [view, set] : m_ViewportTextures) {
        (void)view;
        ImGui_ImplVulkan_RemoveTexture(U64ToDescriptorSet(set));
    }
    m_ViewportTextures.clear();
}

} // namespace Hockey
