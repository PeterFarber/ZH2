#pragma once

#include <cstdint>
#include <filesystem>

#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct SvgInfo {
    uint32_t width = 0;
    uint32_t height = 0;
};

class SvgRasterizer {
public:
    static constexpr int kFallbackSize = 1024;
    static constexpr int kDefaultMaxSize = 2048;

    static Result<SvgInfo> Inspect(const std::filesystem::path& rawPath);
    static Result<TextureAsset> Rasterize(const std::filesystem::path& rawPath,
                                          const TextureImportSettings& settings);
};

} // namespace Hockey
