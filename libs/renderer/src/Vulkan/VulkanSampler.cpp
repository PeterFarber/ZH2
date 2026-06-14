#include "Hockey/Renderer/Vulkan/VulkanSampler.hpp"

#include <algorithm>

namespace Hockey {

VkSampler CreateSampler(const RenderDevice& device, const SamplerDesc& desc) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.minFilter = ToVkFilter(desc.minFilter);
    info.magFilter = ToVkFilter(desc.magFilter);
    info.mipmapMode =
        desc.minFilter == FilterMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = ToVkAddressMode(desc.addressU);
    info.addressModeV = ToVkAddressMode(desc.addressV);
    info.addressModeW = ToVkAddressMode(desc.addressW);
    info.minLod = 0.0f;
    info.maxLod = VK_LOD_CLAMP_NONE;
    info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    const float clampedAniso = std::min(desc.maxAnisotropy, device.gpu.maxSamplerAnisotropy);
    if (clampedAniso > 1.0f && device.gpu.supportsSamplerAnisotropy) {
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = clampedAniso;
    }
    if (desc.compareEnable) {
        info.compareEnable = VK_TRUE;
        info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device.device, &info, nullptr, &sampler) != VK_SUCCESS) {
        HK_CORE_ERROR("Failed to create sampler");
        return VK_NULL_HANDLE;
    }
    return sampler;
}

Status VulkanSamplers::Create(const RenderDevice& device, uint32_t maxAnisotropy) {
    m_Device = &device;

    SamplerDesc linear;
    m_Linear = CreateSampler(device, linear);

    SamplerDesc nearest;
    nearest.minFilter = FilterMode::Nearest;
    nearest.magFilter = FilterMode::Nearest;
    nearest.maxAnisotropy = 1.0f;
    m_Nearest = CreateSampler(device, nearest);

    SamplerDesc aniso;
    aniso.maxAnisotropy = static_cast<float>(maxAnisotropy);
    m_Anisotropic = CreateSampler(device, aniso);

    SamplerDesc shadow;
    shadow.addressU = AddressMode::ClampToBorder;
    shadow.addressV = AddressMode::ClampToBorder;
    shadow.addressW = AddressMode::ClampToBorder;
    shadow.maxAnisotropy = 1.0f;
    shadow.compareEnable = true;
    m_Shadow = CreateSampler(device, shadow);

    SamplerDesc linearClamp;
    linearClamp.addressU = AddressMode::ClampToEdge;
    linearClamp.addressV = AddressMode::ClampToEdge;
    linearClamp.addressW = AddressMode::ClampToEdge;
    linearClamp.maxAnisotropy = 1.0f;
    m_LinearClamp = CreateSampler(device, linearClamp);

    if (m_Linear == VK_NULL_HANDLE || m_Nearest == VK_NULL_HANDLE || m_Anisotropic == VK_NULL_HANDLE ||
        m_Shadow == VK_NULL_HANDLE || m_LinearClamp == VK_NULL_HANDLE) {
        return Status::Fail("Failed to create default samplers");
    }
    device.SetObjectName(reinterpret_cast<uint64_t>(m_Linear), VK_OBJECT_TYPE_SAMPLER, "LinearSampler");
    device.SetObjectName(reinterpret_cast<uint64_t>(m_Anisotropic), VK_OBJECT_TYPE_SAMPLER, "AnisotropicSampler");
    device.SetObjectName(reinterpret_cast<uint64_t>(m_Shadow), VK_OBJECT_TYPE_SAMPLER, "ShadowSampler");
    device.SetObjectName(reinterpret_cast<uint64_t>(m_LinearClamp), VK_OBJECT_TYPE_SAMPLER, "LinearClampSampler");
    return Status::Ok();
}

void VulkanSamplers::Destroy() {
    if (m_Device == nullptr) {
        return;
    }
    for (VkSampler* s : {&m_Linear, &m_Nearest, &m_Anisotropic, &m_Shadow, &m_LinearClamp}) {
        if (*s != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device->device, *s, nullptr);
            *s = VK_NULL_HANDLE;
        }
    }
}

} // namespace Hockey
