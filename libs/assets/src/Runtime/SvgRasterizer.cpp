#include "Hockey/Assets/Runtime/SvgRasterizer.hpp"

#include "Hockey/Assets/Runtime/TextureLoader.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

#include <lunasvg.h>

namespace Hockey {

namespace {

bool IsFinitePositive(float value) {
    return std::isfinite(value) && value > 0.0f;
}

SvgInfo ResolveRasterSize(float sourceWidth, float sourceHeight, int maxSize) {
    float width = IsFinitePositive(sourceWidth) ? sourceWidth : static_cast<float>(SvgRasterizer::kFallbackSize);
    float height = IsFinitePositive(sourceHeight) ? sourceHeight : static_cast<float>(SvgRasterizer::kFallbackSize);

    const float largest = std::max(width, height);
    if (maxSize > 0 && largest > static_cast<float>(maxSize)) {
        const float scale = static_cast<float>(maxSize) / largest;
        width *= scale;
        height *= scale;
    }

    SvgInfo info;
    info.width = std::max(1u, static_cast<uint32_t>(std::lround(width)));
    info.height = std::max(1u, static_cast<uint32_t>(std::lround(height)));
    return info;
}

std::unique_ptr<lunasvg::Document> LoadDocument(const std::filesystem::path& rawPath, std::string& error) {
    std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromFile(rawPath.string());
    if (document == nullptr) {
        error = "failed to parse svg: " + rawPath.string();
    }
    return document;
}

} // namespace

Result<SvgInfo> SvgRasterizer::Inspect(const std::filesystem::path& rawPath) {
    std::string error;
    std::unique_ptr<lunasvg::Document> document = LoadDocument(rawPath, error);
    if (document == nullptr) {
        return Result<SvgInfo>::Fail(error);
    }

    const SvgInfo info = ResolveRasterSize(document->width(), document->height(), kDefaultMaxSize);
    return Result<SvgInfo>::Ok(info);
}

Result<TextureAsset> SvgRasterizer::Rasterize(const std::filesystem::path& rawPath,
                                              const TextureImportSettings& settings) {
    std::string error;
    std::unique_ptr<lunasvg::Document> document = LoadDocument(rawPath, error);
    if (document == nullptr) {
        return Result<TextureAsset>::Fail(error);
    }

    const int maxSize = settings.maxSize > 0 ? settings.maxSize : kDefaultMaxSize;
    const SvgInfo size = ResolveRasterSize(document->width(), document->height(), maxSize);

    lunasvg::Bitmap bitmap =
        document->renderToBitmap(static_cast<int>(size.width), static_cast<int>(size.height), 0x00000000u);
    if (bitmap.isNull()) {
        return Result<TextureAsset>::Fail("failed to rasterize svg: " + rawPath.string());
    }
    bitmap.convertToRGBA();

    TextureAsset asset;
    asset.width = static_cast<uint32_t>(bitmap.width());
    asset.height = static_cast<uint32_t>(bitmap.height());
    asset.channels = 4;
    asset.mipLevels = 1;
    asset.colorSpace = settings.colorSpace;
    asset.semantic = settings.semantic;
    asset.data.resize(static_cast<size_t>(asset.width) * asset.height * 4u);

    const uint8_t* src = bitmap.data();
    std::byte* dst = asset.data.data();
    const size_t rowBytes = static_cast<size_t>(asset.width) * 4u;
    for (uint32_t y = 0; y < asset.height; ++y) {
        std::memcpy(dst + static_cast<size_t>(y) * rowBytes, src + static_cast<size_t>(y) * bitmap.stride(),
                    rowBytes);
    }

    if (settings.generateMipmaps && asset.width > 0 && asset.height > 0) {
        TextureLoader::GenerateMipChain(asset);
    }
    return Result<TextureAsset>::Ok(std::move(asset));
}

} // namespace Hockey
