#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

std::string Base64Encode(const std::vector<uint8_t>& data) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += table[(n >> 18) & 63];
        out += table[(n >> 12) & 63];
        out += table[(n >> 6) & 63];
        out += table[n & 63];
    }
    if (i < data.size()) {
        uint32_t n = data[i] << 16;
        const bool two = (i + 1) < data.size();
        if (two) {
            n |= data[i + 1] << 8;
        }
        out += table[(n >> 18) & 63];
        out += table[(n >> 12) & 63];
        out += two ? table[(n >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

void AppendFloat(std::vector<uint8_t>& bytes, float value) {
    uint8_t raw[4];
    std::memcpy(raw, &value, 4);
    bytes.insert(bytes.end(), raw, raw + 4);
}

void WriteTga(const fs::path& path, uint16_t w, uint16_t h, const std::vector<uint8_t>& rgba) {
    FileSystem::CreateDirectories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    uint8_t header[18] = {};
    header[2] = 2;
    header[12] = static_cast<uint8_t>(w & 0xFF);
    header[14] = static_cast<uint8_t>(h & 0xFF);
    header[16] = 32;
    header[17] = 0x28;
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
        const uint8_t bgra[4] = {rgba[i + 2], rgba[i + 1], rgba[i + 0], rgba[i + 3]};
        out.write(reinterpret_cast<const char*>(bgra), 4);
    }
}

std::string MakeTriangleGltf() {
    // Three POSITION vec3 floats: (0,0,0) (1,0,0) (0,1,0).
    std::vector<uint8_t> buffer;
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 1.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 1.0f);
    AppendFloat(buffer, 0.0f);
    const std::string b64 = Base64Encode(buffer);

    std::string gltf = R"({
  "asset": {"version": "2.0"},
  "scenes": [{"nodes": [0]}],
  "nodes": [{"mesh": 0}],
  "meshes": [{"name": "Triangle", "primitives": [{"attributes": {"POSITION": 0}, "material": 0}]}],
  "materials": [{
    "name": "TriMat",
    "pbrMetallicRoughness": {
      "baseColorFactor": [0.2, 0.4, 0.8, 1.0],
      "metallicFactor": 0.0,
      "roughnessFactor": 0.5,
      "baseColorTexture": {"index": 0}
    }
  }],
  "textures": [{"source": 0, "sampler": 0}],
  "images": [{"uri": "tri_basecolor.tga"}],
  "samplers": [{}],
  "accessors": [{
    "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
    "min": [0.0, 0.0, 0.0], "max": [1.0, 1.0, 0.0]
  }],
  "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 36}],
  "buffers": [{"byteLength": 36, "uri": "data:application/octet-stream;base64,)" +
                       b64 + R"("}]
})";
    return gltf;
}

} // namespace

void RunGltfImportTests() {
    HockeyTest::BeginSuite("GltfImportTests");

    const fs::path workspace = Paths::TempFile("gltf_ws");
    FileSystem::Remove(workspace);
    const fs::path modelDir = workspace / "data" / "raw" / "models";

    FileSystem::WriteTextFile(modelDir / "triangle.gltf", MakeTriangleGltf());
    const std::vector<uint8_t> px = {200, 100, 50, 255, 200, 100, 50, 255, 200, 100, 50, 255, 200, 100, 50, 255};
    WriteTga(modelDir / "tri_basecolor.tga", 2, 2, px);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");

    // Model asset.
    AssetMetadata* modelMeta = manager.Database().FindByRawPath("data/raw/models/triangle.gltf");
    HK_CHECK_MSG(modelMeta != nullptr, "model discovered");
    HK_CHECK_MSG(modelMeta != nullptr && modelMeta->type == AssetType::Model, "type is model");

    // Generated mesh metadata.
    AssetMetadata* meshMeta = manager.Database().FindByRawPath("data/raw/models/triangle.gltf#mesh0");
    HK_CHECK_MSG(meshMeta != nullptr, "mesh sub-asset generated");
    HK_CHECK_MSG(meshMeta != nullptr && meshMeta->type == AssetType::Mesh, "sub-asset is mesh");
    HK_CHECK_MSG(meshMeta != nullptr && meshMeta->name == "Triangle", "mesh name extracted");

    // Generated material metadata.
    AssetMetadata* materialMeta = manager.Database().FindByRawPath("data/raw/models/triangle.gltf#material0");
    HK_CHECK_MSG(materialMeta != nullptr, "material sub-asset generated");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->type == AssetType::Material, "sub-asset is material");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->name == "TriMat", "material name");

    // Texture dependency detection (material -> texture).
    AssetMetadata* texMeta = manager.Database().FindByRawPath("data/raw/models/tri_basecolor.tga");
    HK_CHECK_MSG(texMeta != nullptr, "texture discovered");
    bool materialDependsOnTexture = false;
    if (materialMeta != nullptr && texMeta != nullptr) {
        for (const AssetID dep : materialMeta->dependencies) {
            materialDependsOnTexture = materialDependsOnTexture || dep == texMeta->id;
        }
    }
    HK_CHECK_MSG(materialDependsOnTexture, "material depends on texture");

    // Model references mesh + material.
    bool modelHasMesh = false, modelHasMaterial = false;
    if (modelMeta != nullptr && meshMeta != nullptr && materialMeta != nullptr) {
        for (const AssetID dep : modelMeta->dependencies) {
            modelHasMesh = modelHasMesh || dep == meshMeta->id;
            modelHasMaterial = modelHasMaterial || dep == materialMeta->id;
        }
    }
    HK_CHECK_MSG(modelHasMesh, "model depends on mesh");
    HK_CHECK_MSG(modelHasMaterial, "model depends on material");

    // Cook everything.
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");
    meshMeta = manager.Database().FindByRawPath("data/raw/models/triangle.gltf#mesh0");
    HK_CHECK_MSG(meshMeta != nullptr && meshMeta->cooked, "mesh cooked");

    // Load cooked mesh.
    const AssetID meshId = meshMeta->id;
    Result<std::shared_ptr<MeshAsset>> mesh = manager.Load<MeshAsset>(meshId);
    HK_CHECK_MSG(static_cast<bool>(mesh), "load cooked mesh");
    if (mesh) {
        HK_CHECK_MSG(mesh.value->vertices.size() == 3, "three vertices");
        HK_CHECK_MSG(mesh.value->indices.size() == 3, "three indices");
        HK_CHECK_MSG(mesh.value->submeshes.size() == 1, "one submesh");
        HK_CHECK_MSG(materialMeta != nullptr && mesh.value->submeshes.size() == 1 &&
                         mesh.value->submeshes[0].materialId == materialMeta->id,
                     "submesh material resolved");
        HK_CHECK_NEAR(mesh.value->boundsMax.x, 1.0f, 0.001f);
        HK_CHECK_NEAR(mesh.value->boundsMax.y, 1.0f, 0.001f);
    }

    // Load cooked model.
    Result<std::shared_ptr<ModelAsset>> model = manager.Load<ModelAsset>(modelMeta->id);
    HK_CHECK_MSG(static_cast<bool>(model), "load cooked model");
    if (model) {
        HK_CHECK_MSG(model.value->meshes.size() == 1 && model.value->meshes[0] == meshId, "model lists mesh");
        HK_CHECK_MSG(model.value->materials.size() == 1, "model lists material");
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
