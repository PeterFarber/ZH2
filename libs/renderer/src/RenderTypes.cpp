#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

bool IsDepthFormat(TextureFormat format) {
    return format == TextureFormat::Depth32F || format == TextureFormat::Depth24Stencil8;
}

bool IsStencilFormat(TextureFormat format) {
    return format == TextureFormat::Depth24Stencil8;
}

uint32_t FormatTexelSize(TextureFormat format) {
    switch (format) {
    case TextureFormat::R8:
        return 1;
    case TextureFormat::RG16F:
        return 4;
    case TextureFormat::RGBA8:
    case TextureFormat::RGBA8_SRGB:
    case TextureFormat::BGRA8_SRGB:
        return 4;
    case TextureFormat::RGBA16F:
        return 8;
    case TextureFormat::RGBA32F:
        return 16;
    case TextureFormat::Depth32F:
        return 4;
    case TextureFormat::Depth24Stencil8:
        return 4;
    }
    return 4;
}

const char* TextureFormatToString(TextureFormat format) {
    switch (format) {
    case TextureFormat::RGBA8:
        return "RGBA8";
    case TextureFormat::RGBA8_SRGB:
        return "RGBA8_SRGB";
    case TextureFormat::BGRA8_SRGB:
        return "BGRA8_SRGB";
    case TextureFormat::RGBA16F:
        return "RGBA16F";
    case TextureFormat::RGBA32F:
        return "RGBA32F";
    case TextureFormat::RG16F:
        return "RG16F";
    case TextureFormat::R8:
        return "R8";
    case TextureFormat::Depth32F:
        return "Depth32F";
    case TextureFormat::Depth24Stencil8:
        return "Depth24Stencil8";
    }
    return "Unknown";
}

} // namespace Hockey
