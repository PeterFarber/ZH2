#include "Hockey/Assets/Runtime/AudioLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

std::vector<std::byte> AudioLoader::EncodeCooked(const AudioAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Audio, asset.id, sourceHash, kVersion));
    writer.WriteString(asset.sourceExtension);
    writer.WriteString(asset.debugName);
    writer.Write<uint64_t>(static_cast<uint64_t>(asset.encodedBytes.size()));
    writer.WriteBytes(asset.encodedBytes.data(), asset.encodedBytes.size());
    return buffer;
}

Result<AudioAsset> AudioLoader::DecodeCooked(const std::byte* data, size_t size, AssetID id) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Audio) || header.version != kVersion) {
        return Result<AudioAsset>::Fail("invalid cooked audio header");
    }
    if (id.IsValid() && header.assetID != id.Value()) {
        return Result<AudioAsset>::Fail("cooked audio id mismatch");
    }

    AudioAsset asset;
    asset.id = id.IsValid() ? id : AssetID(header.assetID);
    asset.sourceExtension = reader.ReadString();
    asset.debugName = reader.ReadString();
    const uint64_t byteCount = reader.Read<uint64_t>();
    if (!reader.Ok() || byteCount == 0 || byteCount > reader.Remaining()) {
        return Result<AudioAsset>::Fail("cooked audio truncated");
    }
    asset.encodedBytes.resize(static_cast<size_t>(byteCount));
    reader.ReadBytes(asset.encodedBytes.data(), static_cast<size_t>(byteCount));
    if (!reader.Ok()) {
        return Result<AudioAsset>::Fail("cooked audio read error");
    }
    return Result<AudioAsset>::Ok(std::move(asset));
}

Result<AudioAsset> AudioLoader::LoadCooked(const fs::path& cookedPath, AssetID id) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<AudioAsset>::Fail("cannot open cooked audio: " + cookedPath.string());
    }
    const std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(size > 0 ? static_cast<size_t>(size) : 0);
    if (size > 0) {
        stream.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    if (!stream.good() && !stream.eof()) {
        return Result<AudioAsset>::Fail("failed while reading cooked audio: " + cookedPath.string());
    }
    return DecodeCooked(bytes.data(), bytes.size(), id);
}

} // namespace Hockey
