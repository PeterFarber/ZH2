#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

const char* VkResultString(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    default:
        return "VK_ERROR_<unknown>";
    }
}

VkFormat ToVkFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::RGBA8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::RGBA8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::BGRA8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case TextureFormat::RGBA16F:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::RGBA32F:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TextureFormat::RG16F:
        return VK_FORMAT_R16G16_SFLOAT;
    case TextureFormat::R8:
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::Depth32F:
        return VK_FORMAT_D32_SFLOAT;
    case TextureFormat::Depth24Stencil8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    }
    return VK_FORMAT_R8G8B8A8_UNORM;
}

VkImageUsageFlags ToVkImageUsage(uint32_t usageFlags, TextureFormat format) {
    VkImageUsageFlags result = 0;
    if (usageFlags & TextureUsage_Sampled) {
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usageFlags & TextureUsage_RenderTarget) {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (usageFlags & TextureUsage_DepthStencil) {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (usageFlags & TextureUsage_Storage) {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (usageFlags & TextureUsage_TransferSrc) {
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (usageFlags & TextureUsage_TransferDst) {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    // A render-target color attachment with no explicit depth flag never needs
    // the depth bit; this branch only guards against an empty usage mask.
    if (result == 0) {
        result = IsDepthFormat(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    return result;
}

VkImageAspectFlags FormatAspectMask(TextureFormat format) {
    if (format == TextureFormat::Depth24Stencil8) {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    if (format == TextureFormat::Depth32F) {
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkFilter ToVkFilter(FilterMode mode) {
    return mode == FilterMode::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

VkSamplerAddressMode ToVkAddressMode(AddressMode mode) {
    switch (mode) {
    case AddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return VK_SHADER_STAGE_VERTEX_BIT;
}

VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology) {
    return topology == PrimitiveTopology::LineList ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST
                                                   : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkCullModeFlags ToVkCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::None:
        return VK_CULL_MODE_NONE;
    case CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_BACK_BIT;
}

VkCompareOp ToVkCompareOp(CompareOp op) {
    switch (op) {
    case CompareOp::Never:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::Less:
        return VK_COMPARE_OP_LESS;
    case CompareOp::LessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::Greater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::GreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::Equal:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::Always:
        return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_LESS;
}

} // namespace Hockey
