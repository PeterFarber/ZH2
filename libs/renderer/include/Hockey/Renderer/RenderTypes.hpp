#pragma once

#include <cstdint>

namespace Hockey {

// ---------------------------------------------------------------------------
// Cross-cutting renderer enums shared by buffers, textures, samplers, shaders
// and pipelines. These are backend-agnostic; the Vulkan layer translates them
// into concrete VkFormat / VkImageUsageFlags / etc.
// ---------------------------------------------------------------------------

enum class TextureFormat { RGBA8, RGBA8_SRGB, BGRA8_SRGB, RGBA16F, RGBA32F, RG16F, R8, Depth32F, Depth24Stencil8 };

// Bit flags (combine with operator|). Stored as uint32_t in descriptions.
enum TextureUsageFlags : uint32_t {
    TextureUsage_None = 0,
    TextureUsage_Sampled = 1u << 0,
    TextureUsage_RenderTarget = 1u << 1,
    TextureUsage_DepthStencil = 1u << 2,
    TextureUsage_Storage = 1u << 3,
    TextureUsage_TransferSrc = 1u << 4,
    TextureUsage_TransferDst = 1u << 5
};

enum class FilterMode { Nearest, Linear };

enum class AddressMode { Repeat, ClampToEdge, ClampToBorder };

enum class ShaderStage { Vertex, Fragment, Compute };

enum class AlphaMode { Opaque, Mask, Blend };

enum class PrimitiveTopology { TriangleList, LineList };

enum class CullMode { None, Front, Back };

enum class CompareOp { Never, Less, LessOrEqual, Greater, GreaterOrEqual, Equal, Always };

enum class BlendMode { Opaque, AlphaBlend, Additive };

// True when a format carries depth (and possibly stencil) data.
bool IsDepthFormat(TextureFormat format);
bool IsStencilFormat(TextureFormat format);

// Bytes occupied by a single texel of the given format (uncompressed formats).
uint32_t FormatTexelSize(TextureFormat format);

const char* TextureFormatToString(TextureFormat format);

} // namespace Hockey
