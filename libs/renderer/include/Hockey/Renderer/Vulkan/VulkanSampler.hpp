#pragma once

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Sampler.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

VkSampler CreateSampler(const RenderDevice& device, const SamplerDesc& desc);

// Owns the renderer's common samplers.
class VulkanSamplers {
public:
    Status Create(const RenderDevice& device, uint32_t maxAnisotropy);
    void Destroy();

    VkSampler Linear() const {
        return m_Linear;
    }
    VkSampler Nearest() const {
        return m_Nearest;
    }
    VkSampler Anisotropic() const {
        return m_Anisotropic;
    }
    VkSampler Shadow() const {
        return m_Shadow;
    }
    // Clamp-to-edge linear sampler used by full-screen post-processing passes.
    VkSampler LinearClamp() const {
        return m_LinearClamp;
    }

private:
    const RenderDevice* m_Device = nullptr;
    VkSampler m_Linear = VK_NULL_HANDLE;
    VkSampler m_Nearest = VK_NULL_HANDLE;
    VkSampler m_Anisotropic = VK_NULL_HANDLE;
    VkSampler m_Shadow = VK_NULL_HANDLE;
    VkSampler m_LinearClamp = VK_NULL_HANDLE;
};

} // namespace Hockey
