#pragma once

#include <array>
#include <vector>

#include "Hockey/Renderer/FrameData.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// Owns per-frame-in-flight command pools/buffers and synchronization, plus the
// per-swapchain-image "render finished" semaphores used for presentation.
class VulkanFrameData {
public:
    Status Create(const RenderDevice& device);
    // (Re)creates one present semaphore per swapchain image.
    Status CreatePresentSemaphores(uint32_t imageCount);
    void Destroy();

    FrameData& Frame(uint32_t frameIndex) {
        return m_Frames[frameIndex];
    }
    VkSemaphore RenderFinished(uint32_t imageIndex) const {
        return m_RenderFinished[imageIndex];
    }

private:
    void DestroyPresentSemaphores();

    const RenderDevice* m_Device = nullptr;
    std::array<FrameData, kFramesInFlight> m_Frames{};
    std::vector<VkSemaphore> m_RenderFinished;
};

} // namespace Hockey
