#include "Hockey/Assets/Runtime/ModelLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

std::vector<std::byte> ModelLoader::Encode(const ModelAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Model, asset.id, sourceHash, kVersion));

    writer.WriteString(asset.name);

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.meshes.size()));
    for (const AssetID mesh : asset.meshes) {
        writer.Write<uint64_t>(mesh.Value());
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.materials.size()));
    for (const AssetID material : asset.materials) {
        writer.Write<uint64_t>(material.Value());
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.skeletons.size()));
    for (const AssetID skeleton : asset.skeletons) {
        writer.Write<uint64_t>(skeleton.Value());
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.animations.size()));
    for (const AssetID animation : asset.animations) {
        writer.Write<uint64_t>(animation.Value());
    }
    return buffer;
}

Result<ModelAsset> ModelLoader::Decode(const std::byte* data, size_t size) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Model) ||
        (header.version != 1 && header.version != kVersion)) {
        return Result<ModelAsset>::Fail("invalid cooked model header");
    }

    ModelAsset asset;
    asset.id = AssetID(header.assetID);
    asset.name = reader.ReadString();

    const uint32_t meshCount = reader.Read<uint32_t>();
    if (!reader.Ok() || static_cast<size_t>(meshCount) * sizeof(uint64_t) > reader.Remaining()) {
        return Result<ModelAsset>::Fail("cooked model meshes truncated");
    }
    asset.meshes.reserve(meshCount);
    for (uint32_t i = 0; i < meshCount; ++i) {
        asset.meshes.emplace_back(reader.Read<uint64_t>());
    }

    const uint32_t materialCount = reader.Read<uint32_t>();
    if (!reader.Ok() || static_cast<size_t>(materialCount) * sizeof(uint64_t) > reader.Remaining()) {
        return Result<ModelAsset>::Fail("cooked model materials truncated");
    }
    asset.materials.reserve(materialCount);
    for (uint32_t i = 0; i < materialCount; ++i) {
        asset.materials.emplace_back(reader.Read<uint64_t>());
    }

    if (header.version >= 2) {
        const uint32_t skeletonCount = reader.Read<uint32_t>();
        if (!reader.Ok() || static_cast<size_t>(skeletonCount) * sizeof(uint64_t) > reader.Remaining()) {
            return Result<ModelAsset>::Fail("cooked model skeletons truncated");
        }
        asset.skeletons.reserve(skeletonCount);
        for (uint32_t i = 0; i < skeletonCount; ++i) {
            asset.skeletons.emplace_back(reader.Read<uint64_t>());
        }

        const uint32_t animationCount = reader.Read<uint32_t>();
        if (!reader.Ok() || static_cast<size_t>(animationCount) * sizeof(uint64_t) > reader.Remaining()) {
            return Result<ModelAsset>::Fail("cooked model animations truncated");
        }
        asset.animations.reserve(animationCount);
        for (uint32_t i = 0; i < animationCount; ++i) {
            asset.animations.emplace_back(reader.Read<uint64_t>());
        }
    }

    if (!reader.Ok()) {
        return Result<ModelAsset>::Fail("cooked model read error");
    }
    return Result<ModelAsset>::Ok(std::move(asset));
}

Result<ModelAsset> ModelLoader::LoadCooked(const fs::path& cookedPath) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<ModelAsset>::Fail("cannot open cooked model: " + cookedPath.string());
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
