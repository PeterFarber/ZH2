#pragma once
#include <cstddef>
#include <filesystem>
#include <vector>

#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Owns the cooked texture binary format and raw image decoding (via stb_image).
// The cooker uses Encode/LoadRaw; runtime uses LoadCooked.
class TextureLoader {
public:
    static constexpr uint32_t kVersion = 1;

    // Decodes a raw image file (.png/.jpg/.tga/.bmp/.hdr/...) into RGBA8.
    static Result<TextureAsset> LoadRaw(const std::filesystem::path& rawPath,
                                        const TextureImportSettings& settings);

    // Reads just the dimensions of a raw image without decoding all pixels.
    static bool ReadDimensions(const std::filesystem::path& rawPath, uint32_t& width,
                               uint32_t& height, uint32_t& channels);

    // Appends a full mip chain (down to 1x1) to asset.data and sets mipLevels.
    static void GenerateMipChain(TextureAsset& asset);

    // Cooked binary (header + dimensions + pixel data).
    static std::vector<std::byte> Encode(const TextureAsset& asset, uint64_t sourceHash);
    static Result<TextureAsset> Decode(const std::byte* data, size_t size);
    static Result<TextureAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
