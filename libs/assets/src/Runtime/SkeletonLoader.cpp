#include "Hockey/Assets/Runtime/SkeletonLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

std::vector<std::byte> SkeletonLoader::EncodeCooked(const SkeletonAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Skeleton, asset.id, sourceHash, kVersion));
    writer.WriteString(asset.name);
    writer.Write<uint32_t>(static_cast<uint32_t>(asset.bones.size()));
    for (const SkeletonAssetBone& bone : asset.bones) {
        writer.WriteString(bone.name);
        writer.Write<int32_t>(bone.parentIndex);
        writer.WriteBytes(&bone.inverseBindPose, sizeof(bone.inverseBindPose));
        writer.WriteBytes(&bone.localBindTransform, sizeof(bone.localBindTransform));
    }
    return buffer;
}

Result<SkeletonAsset> SkeletonLoader::DecodeCooked(const std::byte* data, size_t size) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Skeleton) || header.version != kVersion) {
        return Result<SkeletonAsset>::Fail("invalid cooked skeleton header");
    }

    SkeletonAsset asset;
    asset.id = AssetID(header.assetID);
    asset.name = reader.ReadString();
    const uint32_t boneCount = reader.Read<uint32_t>();
    if (!reader.Ok()) {
        return Result<SkeletonAsset>::Fail("cooked skeleton read error");
    }

    asset.bones.reserve(boneCount);
    for (uint32_t i = 0; i < boneCount; ++i) {
        SkeletonAssetBone bone;
        bone.name = reader.ReadString();
        bone.parentIndex = reader.Read<int32_t>();
        reader.ReadBytes(&bone.inverseBindPose, sizeof(bone.inverseBindPose));
        reader.ReadBytes(&bone.localBindTransform, sizeof(bone.localBindTransform));
        asset.bones.push_back(std::move(bone));
    }

    if (!reader.Ok()) {
        return Result<SkeletonAsset>::Fail("cooked skeleton read error");
    }
    return Result<SkeletonAsset>::Ok(std::move(asset));
}

Result<SkeletonAsset> SkeletonLoader::LoadCooked(const fs::path& cookedPath) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<SkeletonAsset>::Fail("cannot open cooked skeleton: " + cookedPath.string());
    }
    const std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(size > 0 ? static_cast<size_t>(size) : 0);
    if (size > 0) {
        stream.read(reinterpret_cast<char*>(bytes.data()), size);
    }
    if (!stream.good() && !stream.eof()) {
        return Result<SkeletonAsset>::Fail("failed while reading cooked skeleton: " + cookedPath.string());
    }
    return DecodeCooked(bytes.data(), bytes.size());
}

} // namespace Hockey
