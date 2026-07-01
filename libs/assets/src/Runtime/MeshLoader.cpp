#include "Hockey/Assets/Runtime/MeshLoader.hpp"

#include "Hockey/Assets/CookedFormat.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

struct MeshVertexV1 {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    glm::vec2 uv0{0.0f};
};

} // namespace

std::vector<std::byte> MeshLoader::Encode(const MeshAsset& asset, uint64_t sourceHash) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Mesh, asset.id, sourceHash, kVersion));

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.vertices.size()));
    if (!asset.vertices.empty()) {
        writer.WriteBytes(asset.vertices.data(), asset.vertices.size() * sizeof(MeshVertex));
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.indices.size()));
    if (!asset.indices.empty()) {
        writer.WriteBytes(asset.indices.data(), asset.indices.size() * sizeof(uint32_t));
    }

    writer.Write<uint32_t>(static_cast<uint32_t>(asset.submeshes.size()));
    for (const MeshSubmesh& submesh : asset.submeshes) {
        writer.Write<uint32_t>(submesh.firstIndex);
        writer.Write<uint32_t>(submesh.indexCount);
        writer.Write<uint64_t>(submesh.materialId.Value());
    }

    writer.Write<float>(asset.boundsMin.x);
    writer.Write<float>(asset.boundsMin.y);
    writer.Write<float>(asset.boundsMin.z);
    writer.Write<float>(asset.boundsMax.x);
    writer.Write<float>(asset.boundsMax.y);
    writer.Write<float>(asset.boundsMax.z);
    return buffer;
}

Result<MeshAsset> MeshLoader::Decode(const std::byte* data, size_t size) {
    BinaryReader reader(data, size);
    const CookedAssetHeader header = reader.ReadHeader();
    if (!reader.Ok() || header.magic != CookedFormat::kMagic ||
        header.assetType != static_cast<uint32_t>(AssetType::Mesh) ||
        (header.version != 1 && header.version != kVersion)) {
        return Result<MeshAsset>::Fail("invalid cooked mesh header");
    }

    MeshAsset asset;
    asset.id = AssetID(header.assetID);

    const uint32_t vertexCount = reader.Read<uint32_t>();
    const size_t encodedVertexSize = header.version == 1 ? sizeof(MeshVertexV1) : sizeof(MeshVertex);
    if (!reader.Ok() || static_cast<size_t>(vertexCount) * encodedVertexSize > reader.Remaining()) {
        return Result<MeshAsset>::Fail("cooked mesh vertices truncated");
    }
    asset.vertices.resize(vertexCount);
    if (header.version == 1) {
        for (MeshVertex& vertex : asset.vertices) {
            MeshVertexV1 legacy;
            reader.ReadBytes(&legacy, sizeof(legacy));
            vertex.position = legacy.position;
            vertex.normal = legacy.normal;
            vertex.tangent = legacy.tangent;
            vertex.uv0 = legacy.uv0;
            vertex.jointIndices = {0, 0, 0, 0};
            vertex.jointWeights = {0.0f, 0.0f, 0.0f, 0.0f};
        }
    } else if (vertexCount > 0) {
        reader.ReadBytes(asset.vertices.data(), vertexCount * sizeof(MeshVertex));
    }

    const uint32_t indexCount = reader.Read<uint32_t>();
    if (!reader.Ok() || static_cast<size_t>(indexCount) * sizeof(uint32_t) > reader.Remaining()) {
        return Result<MeshAsset>::Fail("cooked mesh indices truncated");
    }
    asset.indices.resize(indexCount);
    if (indexCount > 0) {
        reader.ReadBytes(asset.indices.data(), indexCount * sizeof(uint32_t));
    }

    const uint32_t submeshCount = reader.Read<uint32_t>();
    if (!reader.Ok()) {
        return Result<MeshAsset>::Fail("cooked mesh submesh count read error");
    }
    asset.submeshes.reserve(submeshCount);
    for (uint32_t i = 0; i < submeshCount; ++i) {
        MeshSubmesh submesh;
        submesh.firstIndex = reader.Read<uint32_t>();
        submesh.indexCount = reader.Read<uint32_t>();
        submesh.materialId = AssetID(reader.Read<uint64_t>());
        asset.submeshes.push_back(submesh);
    }

    asset.boundsMin.x = reader.Read<float>();
    asset.boundsMin.y = reader.Read<float>();
    asset.boundsMin.z = reader.Read<float>();
    asset.boundsMax.x = reader.Read<float>();
    asset.boundsMax.y = reader.Read<float>();
    asset.boundsMax.z = reader.Read<float>();

    if (!reader.Ok()) {
        return Result<MeshAsset>::Fail("cooked mesh read error");
    }
    return Result<MeshAsset>::Ok(std::move(asset));
}

Result<MeshAsset> MeshLoader::LoadCooked(const fs::path& cookedPath) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<MeshAsset>::Fail("cannot open cooked mesh: " + cookedPath.string());
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
