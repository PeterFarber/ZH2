#include "Hockey/Assets/Runtime/TextureLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"

#include <stb_image.h>

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
std::vector<unsigned char> ReadAllBytes(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return {};
    }
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}
} // namespace

bool TextureLoader::ReadDimensions(const fs::path& rawPath, uint32_t& width, uint32_t& height, uint32_t& channels) {
    const std::vector<unsigned char> bytes = ReadAllBytes(rawPath);
    if (bytes.empty()) {
        return false;
    }
    int w = 0, h = 0, c = 0;
    if (stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &c) == 0) {
        return false;
    }
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    channels = static_cast<uint32_t>(c);
    return true;
}

Result<TextureAsset> TextureLoader::LoadRaw(const fs::path& rawPath, const TextureImportSettings& settings) {
    const std::vector<unsigned char> bytes = ReadAllBytes(rawPath);
    if (bytes.empty()) {
        return Result<TextureAsset>::Fail("cannot read texture file: " + rawPath.string());
    }

    int w = 0, h = 0, sourceChannels = 0;
    stbi_uc* pixels =
        stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &sourceChannels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        return Result<TextureAsset>::Fail("failed to decode texture '" + rawPath.string() + "': " +
                                          std::string(stbi_failure_reason() ? stbi_failure_reason() : "unknown"));
    }

    TextureAsset asset;
    asset.width = static_cast<uint32_t>(w);
    asset.height = static_cast<uint32_t>(h);
    asset.channels = 4;
    asset.mipLevels = 1;
    asset.colorSpace = settings.colorSpace;
    asset.semantic = settings.semantic;

    const size_t byteCount = static_cast<size_t>(w) * static_cast<size_t>(h) * 4u;
    asset.data.resize(byteCount);
    std::memcpy(asset.data.data(), pixels, byteCount);
    stbi_image_free(pixels);

    if (settings.generateMipmaps && asset.width > 0 && asset.height > 0) {
        GenerateMipChain(asset);
    }
    return Result<TextureAsset>::Ok(std::move(asset));
}

void TextureLoader::GenerateMipChain(TextureAsset& asset) {
    if (asset.width == 0 || asset.height == 0) {
        return;
    }
    // asset.data currently holds only mip 0 (RGBA8). Append box-filtered mips.
    uint32_t mipWidth = asset.width;
    uint32_t mipHeight = asset.height;
    const std::byte* previous = asset.data.data();
    uint32_t levels = 1;

    while (mipWidth > 1 || mipHeight > 1) {
        const uint32_t nextWidth = mipWidth > 1 ? mipWidth / 2 : 1;
        const uint32_t nextHeight = mipHeight > 1 ? mipHeight / 2 : 1;

        std::vector<std::byte> next(static_cast<size_t>(nextWidth) * nextHeight * 4u);
        const auto* src = reinterpret_cast<const unsigned char*>(previous);
        auto* dst = reinterpret_cast<unsigned char*>(next.data());
        for (uint32_t y = 0; y < nextHeight; ++y) {
            for (uint32_t x = 0; x < nextWidth; ++x) {
                for (uint32_t ch = 0; ch < 4; ++ch) {
                    const uint32_t x0 = x * 2;
                    const uint32_t y0 = y * 2;
                    const uint32_t x1 = (x0 + 1 < mipWidth) ? x0 + 1 : x0;
                    const uint32_t y1 = (y0 + 1 < mipHeight) ? y0 + 1 : y0;
                    const uint32_t sum = src[(y0 * mipWidth + x0) * 4 + ch] + src[(y0 * mipWidth + x1) * 4 + ch] +
                                         src[(y1 * mipWidth + x0) * 4 + ch] + src[(y1 * mipWidth + x1) * 4 + ch];
                    dst[(y * nextWidth + x) * 4 + ch] = static_cast<unsigned char>(sum / 4);
                }
            }
        }

        const size_t offset = asset.data.size();
        asset.data.insert(asset.data.end(), next.begin(), next.end());
        previous = asset.data.data() + offset;
        mipWidth = nextWidth;
        mipHeight = nextHeight;
        ++levels;
    }
    asset.mipLevels = levels;
}

std::vector<std::byte> TextureLoader::Encode(const TextureAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Texture, asset.id, sourceHash, kVersion));
    writer.Write<uint32_t>(asset.width);
    writer.Write<uint32_t>(asset.height);
    writer.Write<uint32_t>(asset.mipLevels);
    writer.Write<uint32_t>(asset.channels);
    writer.Write<uint32_t>(static_cast<uint32_t>(asset.colorSpace));
    writer.Write<uint32_t>(static_cast<uint32_t>(asset.semantic));
    writer.Write<uint64_t>(static_cast<uint64_t>(asset.data.size()));
    writer.WriteBytes(asset.data.data(), asset.data.size());
    return buffer;
}

Result<TextureAsset> TextureLoader::Decode(const std::byte* data, size_t size) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Texture)) {
        return Result<TextureAsset>::Fail("invalid cooked texture header");
    }

    TextureAsset asset;
    asset.id = AssetID(header.assetID);
    asset.width = reader.Read<uint32_t>();
    asset.height = reader.Read<uint32_t>();
    asset.mipLevels = reader.Read<uint32_t>();
    asset.channels = reader.Read<uint32_t>();
    asset.colorSpace = static_cast<TextureColorSpace>(reader.Read<uint32_t>());
    asset.semantic = static_cast<TextureSemantic>(reader.Read<uint32_t>());
    const uint64_t dataSize = reader.Read<uint64_t>();
    if (!reader.Ok() || dataSize > reader.Remaining()) {
        return Result<TextureAsset>::Fail("cooked texture truncated");
    }
    asset.data.resize(static_cast<size_t>(dataSize));
    if (dataSize > 0) {
        reader.ReadBytes(asset.data.data(), static_cast<size_t>(dataSize));
    }
    if (!reader.Ok()) {
        return Result<TextureAsset>::Fail("cooked texture read error");
    }
    return Result<TextureAsset>::Ok(std::move(asset));
}

Result<TextureAsset> TextureLoader::LoadCooked(const fs::path& cookedPath) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<TextureAsset>::Fail("cannot open cooked texture: " + cookedPath.string());
    }
    const std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(size > 0 ? static_cast<size_t>(size) : 0);
    if (size > 0) {
        stream.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    return Decode(bytes.data(), bytes.size());
}

} // namespace Hockey
