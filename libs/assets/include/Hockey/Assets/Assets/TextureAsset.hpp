#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

enum class TextureColorSpace { Linear, SRGB };

enum class TextureSemantic {
    Unknown,
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    Height,
    Mask
};

// How a raw texture should be interpreted/cooked. Inferred from filename/usage
// hints at import time; persisted alongside the cooked output.
struct TextureImportSettings {
    TextureColorSpace colorSpace = TextureColorSpace::SRGB;
    TextureSemantic semantic = TextureSemantic::Unknown;
    bool generateMipmaps = true;
    bool compress = true;
    bool normalMap = false;
    // Largest cooked dimension. Cooked textures are uncompressed RGBA8 with a
    // full mip chain and no streaming, so an oversized source would balloon both
    // disk and VRAM; clamp to a real-time-friendly budget by default.
    int maxSize = 1024;
};

// Runtime, CPU-side texture. `data` holds tightly packed RGBA8 pixels for the
// base mip (and subsequent mips appended if mipLevels > 1). The renderer
// uploads this to a GPU texture.
struct TextureAsset {
    AssetID id;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t channels = 4;
    TextureColorSpace colorSpace = TextureColorSpace::SRGB;
    TextureSemantic semantic = TextureSemantic::Unknown;
    std::vector<std::byte> data;
};

std::string TextureColorSpaceToString(TextureColorSpace value);
TextureColorSpace TextureColorSpaceFromString(const std::string& value);
std::string TextureSemanticToString(TextureSemantic value);
TextureSemantic TextureSemanticFromString(const std::string& value);

} // namespace Hockey
