#pragma once

// Single place that pulls in the Vulkan function loader. Every other Vulkan
// translation unit includes this header (never <vulkan/vulkan.h> directly) so
// that all entry points come from volk.
#include <volk.h>

#include <cstdint>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

const char* VkResultString(VkResult result);

// Number of frames the CPU may work ahead of the GPU.
inline constexpr uint32_t kFramesInFlight = 2;

// ---------------------------------------------------------------------------
// Result checking helpers.
//   VK_CHECK    - use inside functions returning Status; bails out on failure.
//   VK_VERIFY   - hard programmer-error assertion (debug only abort).
// ---------------------------------------------------------------------------

#define VK_CHECK(expr)                                                                                                 \
    do {                                                                                                               \
        const VkResult hk_vk_result = (expr);                                                                          \
        if (hk_vk_result != VK_SUCCESS) {                                                                              \
            HK_CORE_ERROR("Vulkan call failed ({}): {}", ::Hockey::VkResultString(hk_vk_result), #expr);               \
            return ::Hockey::Status::Fail(std::string("Vulkan call failed: ") + #expr);                                \
        }                                                                                                              \
    } while (false)

#define VK_VERIFY(expr)                                                                                                \
    do {                                                                                                               \
        const VkResult hk_vk_result = (expr);                                                                          \
        if (hk_vk_result != VK_SUCCESS) {                                                                              \
            HK_CORE_CRITICAL("Vulkan call failed ({}): {}", ::Hockey::VkResultString(hk_vk_result), #expr);            \
        }                                                                                                              \
    } while (false)

// ---------------------------------------------------------------------------
// Enum translation: backend-agnostic RenderTypes -> Vulkan.
// ---------------------------------------------------------------------------

VkFormat ToVkFormat(TextureFormat format);
VkImageUsageFlags ToVkImageUsage(uint32_t usageFlags, TextureFormat format);
VkImageAspectFlags FormatAspectMask(TextureFormat format);
VkFilter ToVkFilter(FilterMode mode);
VkSamplerAddressMode ToVkAddressMode(AddressMode mode);
VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage);
VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology);
VkCullModeFlags ToVkCullMode(CullMode mode);
VkCompareOp ToVkCompareOp(CompareOp op);

} // namespace Hockey
