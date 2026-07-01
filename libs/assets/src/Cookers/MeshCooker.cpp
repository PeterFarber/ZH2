#include "Hockey/Assets/Cookers/MeshCooker.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Importers/GltfImporter.hpp"
#include "Hockey/Assets/Runtime/MeshLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>
#include <fstream>
#include <vector>

#include <meshoptimizer.h>
#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

struct MeshDescriptor {
    fs::path sourceModel;
    size_t meshIndex = 0;
    std::vector<fs::path> materialPaths;
    fs::path skeletonPath;
};

Result<MeshDescriptor> LoadMeshDescriptor(const fs::path& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const YAML::Exception& e) {
        return Result<MeshDescriptor>::Fail(std::string("mesh descriptor parse error: ") + e.what());
    }

    const YAML::Node node = root["Mesh"];
    if (!node || !node.IsMap()) {
        return Result<MeshDescriptor>::Fail("mesh descriptor missing top-level 'Mesh' map: " + path.string());
    }
    if (!node["SourceModel"] || !node["MeshIndex"]) {
        return Result<MeshDescriptor>::Fail("mesh descriptor missing SourceModel or MeshIndex: " + path.string());
    }

    MeshDescriptor descriptor;
    descriptor.sourceModel = node["SourceModel"].as<std::string>();
    descriptor.meshIndex = node["MeshIndex"].as<size_t>();
    if (const YAML::Node materials = node["Materials"]; materials && materials.IsSequence()) {
        for (const YAML::Node material : materials) {
            descriptor.materialPaths.push_back(material.as<std::string>());
        }
    }
    if (const YAML::Node skeleton = node["Skeleton"]) {
        descriptor.skeletonPath = skeleton.as<std::string>();
    }
    return Result<MeshDescriptor>::Ok(std::move(descriptor));
}

void Optimize(MeshAsset& mesh) {
    const size_t indexCount = mesh.indices.size();
    const size_t vertexCount = mesh.vertices.size();
    if (indexCount == 0 || vertexCount == 0) {
        return;
    }

    // 1. Weld duplicate vertices and renumber indices (preserves index order, so
    //    submesh firstIndex/indexCount ranges remain valid).
    std::vector<unsigned int> remap(vertexCount);
    const size_t uniqueVertices = meshopt_generateVertexRemap(remap.data(), mesh.indices.data(), indexCount,
                                                              mesh.vertices.data(), vertexCount, sizeof(MeshVertex));

    std::vector<MeshVertex> vertices(uniqueVertices);
    meshopt_remapVertexBuffer(vertices.data(), mesh.vertices.data(), vertexCount, sizeof(MeshVertex), remap.data());
    std::vector<uint32_t> indices(indexCount);
    meshopt_remapIndexBuffer(indices.data(), mesh.indices.data(), indexCount, remap.data());

    // 2. Optimize vertex-cache locality per submesh (each submesh is one draw
    //    call, so its index range must be optimized independently).
    for (const MeshSubmesh& submesh : mesh.submeshes) {
        if (submesh.indexCount == 0) {
            continue;
        }
        meshopt_optimizeVertexCache(indices.data() + submesh.firstIndex, indices.data() + submesh.firstIndex,
                                    submesh.indexCount, uniqueVertices);
    }

    // 3. Reorder the vertex buffer to match index access order (renumbers
    //    indices in place; index order/count unchanged).
    std::vector<MeshVertex> fetched(uniqueVertices);
    meshopt_optimizeVertexFetch(fetched.data(), indices.data(), indexCount, vertices.data(), uniqueVertices,
                                sizeof(MeshVertex));

    mesh.vertices = std::move(fetched);
    mesh.indices = std::move(indices);
}

} // namespace

CookResult MeshCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    fs::path modelRawPath;
    size_t meshIndex = 0;
    bool isMesh = false;
    std::vector<fs::path> materialPaths;
    fs::path skeletonPath;
    const bool legacySubAsset = GltfImporter::ParseSubAsset(metadata.rawPath, modelRawPath, meshIndex, isMesh);
    if (legacySubAsset) {
        if (!isMesh) {
            result.success = false;
            result.error = "mesh cooker got a non-mesh glTF sub-asset path: " + metadata.rawPath.generic_string();
            return result;
        }
    } else {
        Result<MeshDescriptor> descriptor = LoadMeshDescriptor(context.projectRoot / metadata.rawPath);
        if (!descriptor) {
            result.success = false;
            result.error = descriptor.error;
            return result;
        }
        modelRawPath = descriptor.value.sourceModel;
        meshIndex = descriptor.value.meshIndex;
        materialPaths = std::move(descriptor.value.materialPaths);
        skeletonPath = std::move(descriptor.value.skeletonPath);
    }

    const fs::path gltfAbsolute = context.projectRoot / modelRawPath;
    Result<GltfScene> scene = GltfLoader::Load(gltfAbsolute);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }
    if (meshIndex >= scene.value.meshes.size()) {
        result.success = false;
        result.error = "glTF mesh index out of range: " + metadata.rawPath.generic_string();
        return result;
    }

    const GltfMeshData& source = scene.value.meshes[meshIndex];

    MeshAsset asset;
    asset.id = metadata.id;
    asset.vertices = source.vertices;
    asset.indices = source.indices;
    asset.submeshes = source.submeshes;
    asset.boundsMin = source.boundsMin;
    asset.boundsMax = source.boundsMax;

    // Resolve submesh materials to the generated material sub-asset AssetIDs.
    for (size_t i = 0; i < asset.submeshes.size(); ++i) {
        const int materialIndex = source.submeshMaterialIndex[i];
        if (materialIndex < 0 || context.database == nullptr) {
            continue;
        }
        fs::path materialPath;
        if (legacySubAsset) {
            materialPath = GltfImporter::MaterialSubAssetPath(modelRawPath, static_cast<size_t>(materialIndex));
        } else if (static_cast<size_t>(materialIndex) < materialPaths.size()) {
            materialPath = materialPaths[static_cast<size_t>(materialIndex)];
        }
        if (materialPath.empty()) {
            continue;
        }
        if (const AssetMetadata* materialMeta = context.database->FindByRawPath(materialPath)) {
            asset.submeshes[i].materialId = materialMeta->id;
            if (std::find(result.dependencies.begin(), result.dependencies.end(), materialMeta->id) ==
                result.dependencies.end()) {
                result.dependencies.push_back(materialMeta->id);
            }
        }
    }
    if (!skeletonPath.empty() && context.database != nullptr) {
        if (const AssetMetadata* skeletonMeta = context.database->FindByRawPath(skeletonPath)) {
            if (std::find(result.dependencies.begin(), result.dependencies.end(), skeletonMeta->id) ==
                result.dependencies.end()) {
                result.dependencies.push_back(skeletonMeta->id);
            }
        }
    }

    Optimize(asset);

    const uint64_t sourceHash = AssetHash::HashFile(gltfAbsolute);
    const std::vector<std::byte> cooked = MeshLoader::Encode(asset, sourceHash);

    const fs::path cookedAbsolute = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(AssetType::Mesh) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Mesh));

    const Status created = FileSystem::CreateDirectories(cookedAbsolute.parent_path());
    if (!created) {
        result.success = false;
        result.error = created.error;
        return result;
    }
    std::ofstream out(cookedAbsolute, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        result.success = false;
        result.error = "cannot write cooked mesh: " + cookedAbsolute.string();
        return result;
    }
    out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
