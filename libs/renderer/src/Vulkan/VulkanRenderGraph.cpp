#include "Hockey/Renderer/Vulkan/VulkanRenderGraph.hpp"

#include <vector>

#include "Hockey/Renderer/RenderContext.hpp"
#include "Hockey/Renderer/Vulkan/VulkanDebug.hpp"

namespace Hockey {

namespace {

VkAttachmentLoadOp ToVkLoadOp(LoadOp op) {
    switch (op) {
    case LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp ToVkStoreOp(StoreOp op) {
    switch (op) {
    case StoreOp::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreOp::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

uint32_t ResourceSlot(TextureHandle handle) {
    return handle.id - 1; // caller guarantees handle.IsValid()
}

} // namespace

Status VulkanRenderGraph::Create(const RenderDevice& device, const VulkanCommand& command) {
    m_Device = &device;
    m_Command = &command;
    return Status::Ok();
}

void VulkanRenderGraph::Destroy() {
    if (m_Device != nullptr) {
        for (VulkanTexture& tex : m_Transient) {
            if (tex.IsValid()) {
                DestroyTexture(*m_Device, tex);
            }
        }
    }
    m_Transient.clear();
    m_Imported.clear();
    m_IsImported.clear();
    m_Device = nullptr;
    m_Command = nullptr;
}

Status VulkanRenderGraph::Realize(RenderGraph& graph) {
    if (m_Device == nullptr || m_Command == nullptr) {
        return Status::Fail("VulkanRenderGraph::Realize called before Create");
    }

    const size_t count = graph.ResourceCount();

    // Free transient images that no longer have a slot (graph shrank).
    for (size_t i = count; i < m_Transient.size(); ++i) {
        if (m_Transient[i].IsValid()) {
            DestroyTexture(*m_Device, m_Transient[i]);
        }
    }
    m_Transient.resize(count);
    m_Imported.resize(count);
    m_IsImported.assign(count, false);

    for (size_t i = 0; i < count; ++i) {
        const TextureHandle handle{static_cast<uint32_t>(i + 1)};
        const RenderGraph::Resource* res = graph.GetResource(handle);
        if (res == nullptr) {
            continue;
        }
        if (res->imported) {
            m_IsImported[i] = true;
            if (m_Transient[i].IsValid()) {
                DestroyTexture(*m_Device, m_Transient[i]);
            }
            continue;
        }

        const bool needsRealloc =
            res->dirty || !m_Transient[i].IsValid() || m_Transient[i].extent.width != res->desc.width ||
            m_Transient[i].extent.height != res->desc.height || m_Transient[i].logicalFormat != res->desc.format;
        if (needsRealloc) {
            if (m_Transient[i].IsValid()) {
                DestroyTexture(*m_Device, m_Transient[i]);
            }
            m_Transient[i] = CreateTexture(*m_Device, *m_Command, res->desc);
            if (!m_Transient[i].IsValid()) {
                return Status::Fail("Failed to realize transient render graph texture: " + res->desc.debugName);
            }
        }
    }

    graph.MarkResourcesRealized();
    return Status::Ok();
}

void VulkanRenderGraph::BindImported(TextureHandle handle, const VulkanTexture& texture) {
    if (!handle.IsValid()) {
        return;
    }
    const uint32_t slot = ResourceSlot(handle);
    if (slot >= m_Imported.size()) {
        return;
    }
    m_Imported[slot] = texture;
    m_IsImported[slot] = true;
}

VulkanTexture* VulkanRenderGraph::Resolve(TextureHandle handle) {
    if (!handle.IsValid()) {
        return nullptr;
    }
    const uint32_t slot = ResourceSlot(handle);
    if (slot >= m_IsImported.size()) {
        return nullptr;
    }
    return m_IsImported[slot] ? &m_Imported[slot] : &m_Transient[slot];
}

void VulkanRenderGraph::Bind(RenderContext& context) {
    context.resources = this;
    context.beginPass = [this](RenderContext& ctx, const RenderPassDesc& desc) { BeginPass(ctx, desc); };
    context.endPass = [this](RenderContext& ctx, const RenderPassDesc& desc) { EndPass(ctx, desc); };
}

void VulkanRenderGraph::BeginPass(RenderContext& context, const RenderPassDesc& desc) {
    VkCommandBuffer cmd = context.cmd;
    BeginDebugLabel(cmd, desc.name.empty() ? "Pass" : desc.name.c_str());

    // Move read resources into a shader-readable layout.
    for (const TextureHandle read : desc.reads) {
        VulkanTexture* tex = Resolve(read);
        if (tex == nullptr || !tex->IsValid()) {
            continue;
        }
        if (tex->currentLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            TransitionImage(cmd, tex->image, tex->currentLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex->aspect,
                            0, tex->mipLevels);
            tex->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    if (desc.type != RenderPassType::Graphics) {
        return; // compute/transfer record their own work without a render scope
    }

    std::vector<VkRenderingAttachmentInfo> colorInfos;
    colorInfos.reserve(desc.colorAttachments.size());
    VkExtent2D area = context.renderArea;

    for (const ColorAttachment& color : desc.colorAttachments) {
        VulkanTexture* tex = Resolve(color.texture);
        if (tex == nullptr || !tex->IsValid()) {
            continue;
        }
        if (tex->currentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            TransitionImage(cmd, tex->image, tex->currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_ASPECT_COLOR_BIT);
            tex->currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        area = tex->extent;

        VkRenderingAttachmentInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.imageView = tex->view;
        info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.loadOp = ToVkLoadOp(color.loadOp);
        info.storeOp = ToVkStoreOp(color.storeOp);
        info.clearValue.color = {{color.clearColor.r, color.clearColor.g, color.clearColor.b, color.clearColor.a}};
        colorInfos.push_back(info);
    }

    VkRenderingAttachmentInfo depthInfo{};
    bool hasDepth = false;
    if (desc.depth.Enabled()) {
        VulkanTexture* tex = Resolve(desc.depth.texture);
        if (tex != nullptr && tex->IsValid()) {
            if (tex->currentLayout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
                TransitionImage(cmd, tex->image, tex->currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                tex->aspect);
                tex->currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            }
            area = tex->extent;
            depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthInfo.imageView = tex->view;
            depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthInfo.loadOp = ToVkLoadOp(desc.depth.loadOp);
            depthInfo.storeOp = ToVkStoreOp(desc.depth.storeOp);
            depthInfo.clearValue.depthStencil = {desc.depth.clearDepth, desc.depth.clearStencil};
            hasDepth = true;
        }
    }

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea = {{0, 0}, area};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = static_cast<uint32_t>(colorInfos.size());
    rendering.pColorAttachments = colorInfos.empty() ? nullptr : colorInfos.data();
    rendering.pDepthAttachment = hasDepth ? &depthInfo : nullptr;
    vkCmdBeginRendering(cmd, &rendering);
}

void VulkanRenderGraph::EndPass(RenderContext& context, const RenderPassDesc& desc) {
    if (desc.type == RenderPassType::Graphics) {
        vkCmdEndRendering(context.cmd);
    }
    EndDebugLabel(context.cmd);
}

} // namespace Hockey
