#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
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
    return std::string(R"({
  "asset": {"version": "2.0"},
  "meshes": [{"name": "M", "primitives": [{"attributes": {"POSITION": 0}, "material": 0}]}],
  "materials": [{"name": "Mat", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}],
  "textures": [{"source": 0}],
  "images": [{"uri": "model_basecolor.tga"}],
  "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"}],
  "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 36}],
  "buffers": [{"byteLength": 36, "uri": "data:application/octet-stream;base64,)") +
           b64 + R"("}]
})";
}

const char* kMaterialYaml = R"(Material:
  Name: Floor
  BaseColor: [1, 1, 1, 1]
  Textures:
    BaseColor: data/raw/textures/floor_basecolor.tga
)";

} // namespace

void RunDependencyTests() {
    HockeyTest::BeginSuite("DependencyTests");

    const fs::path workspace = Paths::TempFile("dependency_ws");
    FileSystem::Remove(workspace);
    const fs::path raw = workspace / "data" / "raw";

    const std::vector<uint8_t> px = {180, 180, 180, 255, 180, 180, 180, 255, 180, 180, 180, 255, 180, 180, 180, 255};
    WriteTga(raw / "textures" / "floor_basecolor.tga", 2, 2, px);
    FileSystem::WriteTextFile(raw / "materials" / "floor.material.yaml", kMaterialYaml);

    WriteTga(raw / "models" / "model_basecolor.tga", 2, 2, px);
    FileSystem::WriteTextFile(raw / "models" / "model.gltf", MakeTriangleGltf());

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");

    AssetDatabase& db = manager.Database();

    // --- material depends on texture ---
    AssetMetadata* matMeta = db.FindByRawPath("data/raw/materials/floor.material.yaml");
    AssetMetadata* texMeta = db.FindByRawPath("data/raw/textures/floor_basecolor.tga");
    HK_CHECK_MSG(matMeta != nullptr && texMeta != nullptr, "material + texture present");
    bool matDependsTex = false;
    if (matMeta != nullptr && texMeta != nullptr) {
        for (const AssetID dep : matMeta->dependencies) {
            matDependsTex = matDependsTex || dep == texMeta->id;
        }
    }
    HK_CHECK_MSG(matDependsTex, "material depends on texture");

    // Texture records the material as a dependent (graph is bidirectional).
    bool texHasMatDependent = false;
    if (matMeta != nullptr && texMeta != nullptr) {
        for (const AssetID d : texMeta->dependents) {
            texHasMatDependent = texHasMatDependent || d == matMeta->id;
        }
    }
    HK_CHECK_MSG(texHasMatDependent, "texture lists material as dependent");

    // --- model depends on mesh + material; mesh/material chain to texture ---
    AssetMetadata* modelMeta = db.FindByRawPath("data/raw/models/model.gltf");
    AssetMetadata* meshMeta = db.FindByRawPath("data/raw/meshes/model/M.mesh.yaml");
    AssetMetadata* modelMatMeta = db.FindByRawPath("data/raw/materials/model/Mat.material.yaml");
    AssetMetadata* modelTexMeta = db.FindByRawPath("data/raw/textures/model/model_basecolor.tga");
    HK_CHECK_MSG(modelMeta != nullptr && meshMeta != nullptr && modelMatMeta != nullptr && modelTexMeta != nullptr,
                 "model graph present");
    bool modelHasMesh = false, modelHasMat = false;
    if (modelMeta != nullptr) {
        for (const AssetID dep : modelMeta->dependencies) {
            modelHasMesh = modelHasMesh || (meshMeta != nullptr && dep == meshMeta->id);
            modelHasMat = modelHasMat || (modelMatMeta != nullptr && dep == modelMatMeta->id);
        }
    }
    HK_CHECK_MSG(modelHasMesh, "model depends on mesh");
    HK_CHECK_MSG(modelHasMat, "model depends on material");
    bool modelMatDependsTex = false;
    if (modelMatMeta != nullptr && modelTexMeta != nullptr) {
        for (const AssetID dep : modelMatMeta->dependencies) {
            modelMatDependsTex = modelMatDependsTex || dep == modelTexMeta->id;
        }
    }
    HK_CHECK_MSG(modelMatDependsTex, "model material depends on texture");

    // --- dirty dependency marks dependent dirty ---
    // Clear dirty flags (cook already did), then dirty the texture and propagate.
    HK_CHECK_MSG(matMeta != nullptr && !matMeta->dirty, "material clean after cook");
    if (texMeta != nullptr) {
        db.MarkDirtyWithDependents(texMeta->id);
    }
    matMeta = db.FindByRawPath("data/raw/materials/floor.material.yaml");
    texMeta = db.FindByRawPath("data/raw/textures/floor_basecolor.tga");
    HK_CHECK_MSG(texMeta != nullptr && texMeta->dirty, "texture marked dirty");
    HK_CHECK_MSG(matMeta != nullptr && matMeta->dirty, "dependent material marked dirty");

    // Transitive: dirtying the model texture should cascade to material (and model).
    if (modelTexMeta != nullptr) {
        db.MarkDirtyWithDependents(modelTexMeta->id);
    }
    modelMatMeta = db.FindByRawPath("data/raw/materials/model/Mat.material.yaml");
    HK_CHECK_MSG(modelMatMeta != nullptr && modelMatMeta->dirty, "transitive dependent material dirty");

    // --- missing dependency is reported ---
    FileSystem::Remove(raw / "textures" / "floor_basecolor.tga");
    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "rediscover after delete");
    std::vector<AssetValidationIssue> issues;
    const Status validation = manager.ValidateReferences(&issues);
    HK_CHECK_MSG(!validation, "validation fails with missing dependency");
    bool reportedMissing = false;
    for (const AssetValidationIssue& issue : issues) {
        if (issue.error) {
            reportedMissing = true;
        }
    }
    HK_CHECK_MSG(reportedMissing, "missing dependency reported");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
