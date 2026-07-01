#include "Hockey/Assets/Runtime/AnimationLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

template <typename Key> void WriteVec3Keys(BinaryWriter& writer, const std::vector<Key>& keys) {
    writer.Write<uint32_t>(static_cast<uint32_t>(keys.size()));
    for (const Key& key : keys) {
        writer.Write<float>(key.timeSeconds);
        writer.WriteBytes(&key.value, sizeof(key.value));
    }
}

template <typename Key> bool ReadVec3Keys(BinaryReader& reader, std::vector<Key>& keys) {
    const uint32_t count = reader.Read<uint32_t>();
    if (!reader.Ok()) {
        return false;
    }
    keys.resize(count);
    for (Key& key : keys) {
        key.timeSeconds = reader.Read<float>();
        reader.ReadBytes(&key.value, sizeof(key.value));
    }
    return reader.Ok();
}

} // namespace

std::vector<std::byte> AnimationLoader::EncodeCooked(const AnimationAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Animation, asset.id, sourceHash, kVersion));
    writer.WriteString(asset.name);
    writer.Write<uint64_t>(asset.skeletonAsset.Value());
    writer.Write<float>(asset.durationSeconds);
    writer.Write<uint8_t>(asset.looping ? 1u : 0u);

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.tracks.size()));
    for (const AnimationBoneTrack& track : asset.tracks) {
        writer.Write<int32_t>(track.boneIndex);
        WriteVec3Keys(writer, track.translations);

        writer.Write<uint32_t>(static_cast<uint32_t>(track.rotations.size()));
        for (const AnimationAssetRotationKey& key : track.rotations) {
            writer.Write<float>(key.timeSeconds);
            writer.WriteBytes(&key.value, sizeof(key.value));
        }

        WriteVec3Keys(writer, track.scales);
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.events.size()));
    for (const AnimationAssetEvent& event : asset.events) {
        writer.Write<float>(event.timeSeconds);
        writer.WriteString(event.name);
    }

    return buffer;
}

Result<AnimationAsset> AnimationLoader::DecodeCooked(const std::byte* data, size_t size) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Animation) || header.version != kVersion) {
        return Result<AnimationAsset>::Fail("invalid cooked animation header");
    }

    AnimationAsset asset;
    asset.id = AssetID(header.assetID);
    asset.name = reader.ReadString();
    asset.skeletonAsset = AssetID(reader.Read<uint64_t>());
    asset.durationSeconds = reader.Read<float>();
    asset.looping = reader.Read<uint8_t>() != 0;

    const uint32_t trackCount = reader.Read<uint32_t>();
    if (!reader.Ok()) {
        return Result<AnimationAsset>::Fail("cooked animation read error");
    }
    asset.tracks.resize(trackCount);
    for (AnimationBoneTrack& track : asset.tracks) {
        track.boneIndex = reader.Read<int32_t>();
        if (!ReadVec3Keys(reader, track.translations)) {
            return Result<AnimationAsset>::Fail("cooked animation translation keys truncated");
        }

        const uint32_t rotationCount = reader.Read<uint32_t>();
        track.rotations.resize(rotationCount);
        for (AnimationAssetRotationKey& key : track.rotations) {
            key.timeSeconds = reader.Read<float>();
            reader.ReadBytes(&key.value, sizeof(key.value));
        }

        if (!ReadVec3Keys(reader, track.scales)) {
            return Result<AnimationAsset>::Fail("cooked animation scale keys truncated");
        }
    }

    const uint32_t eventCount = reader.Read<uint32_t>();
    asset.events.resize(eventCount);
    for (AnimationAssetEvent& event : asset.events) {
        event.timeSeconds = reader.Read<float>();
        event.name = reader.ReadString();
    }

    if (!reader.Ok()) {
        return Result<AnimationAsset>::Fail("cooked animation read error");
    }
    return Result<AnimationAsset>::Ok(std::move(asset));
}

Result<AnimationAsset> AnimationLoader::LoadCooked(const fs::path& cookedPath) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<AnimationAsset>::Fail("cannot open cooked animation: " + cookedPath.string());
    }
    const std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(size > 0 ? static_cast<size_t>(size) : 0);
    if (size > 0) {
        stream.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    if (!stream.good() && !stream.eof()) {
        return Result<AnimationAsset>::Fail("failed while reading cooked animation: " + cookedPath.string());
    }
    return DecodeCooked(bytes.data(), bytes.size());
}

} // namespace Hockey
