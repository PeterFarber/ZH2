#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
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

void AppendU16(std::vector<uint8_t>& bytes, uint16_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
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

std::string MakeSkinnedTriangleGltf() {
    std::vector<uint8_t> buffer;

    // POSITION (3 x vec3)
    const float positions[3][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    for (const auto& p : positions) {
        AppendFloat(buffer, p[0]);
        AppendFloat(buffer, p[1]);
        AppendFloat(buffer, p[2]);
    }

    // JOINTS_0 (3 x u16vec4)
    const uint16_t joints[3][4] = {{0, 1, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}};
    for (const auto& j : joints) {
        AppendU16(buffer, j[0]);
        AppendU16(buffer, j[1]);
        AppendU16(buffer, j[2]);
        AppendU16(buffer, j[3]);
    }

    // WEIGHTS_0 (3 x vec4)
    const float weights[3][4] = {{0.75f, 0.25f, 0.0f, 0.0f},
                                 {0.5f, 0.5f, 0.0f, 0.0f},
                                 {1.0f, 0.0f, 0.0f, 0.0f}};
    for (const auto& w : weights) {
        AppendFloat(buffer, w[0]);
        AppendFloat(buffer, w[1]);
        AppendFloat(buffer, w[2]);
        AppendFloat(buffer, w[3]);
    }

    // INDICES (3 x u16), then two bytes of padding for MAT4 alignment.
    AppendU16(buffer, 0);
    AppendU16(buffer, 1);
    AppendU16(buffer, 2);
    AppendU16(buffer, 0);

    // Inverse bind matrices for Root and Hand.
    const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const float inverseHand[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 1};
    for (const float value : identity) {
        AppendFloat(buffer, value);
    }
    for (const float value : inverseHand) {
        AppendFloat(buffer, value);
    }

    // Animation input times and output translations for the Hand joint.
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 1.0f);
    AppendFloat(buffer, 1.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 2.0f);
    AppendFloat(buffer, 0.0f);
    AppendFloat(buffer, 0.0f);

    const std::string b64 = Base64Encode(buffer);
    return std::string(R"({
  "asset": {"version": "2.0"},
  "scenes": [{"nodes": [0, 1]}],
  "nodes": [
    {"name": "SkinTriNode", "mesh": 0, "skin": 0},
    {"name": "Root", "children": [2]},
    {"name": "Hand", "translation": [1.0, 0.0, 0.0]}
  ],
  "skins": [{"name": "Armature", "joints": [1, 2], "inverseBindMatrices": 4, "skeleton": 1}],
  "meshes": [{"name": "SkinTri", "primitives": [{"attributes": {"POSITION": 0, "JOINTS_0": 1, "WEIGHTS_0": 2}, "indices": 3}]}],
  "animations": [{
    "name": "Wave",
    "samplers": [{"input": 5, "output": 6, "interpolation": "LINEAR"}],
    "channels": [{"sampler": 0, "target": {"node": 2, "path": "translation"}}]
  }],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0,0,0], "max": [1,1,0]},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "VEC4"},
    {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC4"},
    {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"},
    {"bufferView": 4, "componentType": 5126, "count": 2, "type": "MAT4"},
    {"bufferView": 5, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0.0], "max": [1.0]},
    {"bufferView": 6, "componentType": 5126, "count": 2, "type": "VEC3"}
  ],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 24},
    {"buffer": 0, "byteOffset": 60, "byteLength": 48},
    {"buffer": 0, "byteOffset": 108, "byteLength": 6},
    {"buffer": 0, "byteOffset": 116, "byteLength": 128},
    {"buffer": 0, "byteOffset": 244, "byteLength": 8},
    {"buffer": 0, "byteOffset": 252, "byteLength": 24}
  ],
  "buffers": [{"byteLength": 276, "uri": "data:application/octet-stream;base64,)") +
           b64 + R"("}]
})";
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

    const fs::path canonicalTexturePath = "data/raw/textures/triangle/tri_basecolor.tga";
    const fs::path canonicalMaterialPath = "data/raw/materials/triangle/TriMat.material.yaml";
    const fs::path canonicalMeshPath = "data/raw/meshes/triangle/Triangle.mesh.yaml";

    // Generated mesh metadata and descriptor are routed through the raw meshes folder.
    AssetMetadata* meshMeta = manager.Database().FindByRawPath(canonicalMeshPath);
    HK_CHECK_MSG(meshMeta != nullptr, "mesh raw descriptor generated");
    HK_CHECK_MSG(meshMeta != nullptr && meshMeta->type == AssetType::Mesh, "sub-asset is mesh");
    HK_CHECK_MSG(meshMeta != nullptr && meshMeta->name == "Triangle", "mesh name extracted");
    HK_CHECK_MSG(FileSystem::Exists(workspace / canonicalMeshPath), "mesh descriptor written under raw meshes");

    // Generated material metadata and authored YAML are routed through the raw materials folder.
    AssetMetadata* materialMeta = manager.Database().FindByRawPath(canonicalMaterialPath);
    HK_CHECK_MSG(materialMeta != nullptr, "material raw asset generated");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->type == AssetType::Material, "sub-asset is material");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->name == "TriMat", "material name");
    Result<MaterialSource> materialSource = MaterialSerializer::LoadFile(workspace / canonicalMaterialPath);
    HK_CHECK_MSG(static_cast<bool>(materialSource), "generated material yaml parses");
    if (materialSource) {
        HK_CHECK_MSG(materialSource.value.baseColorTexture == canonicalTexturePath.generic_string(),
                     "generated material references canonical texture path");
    }

    // Texture dependency detection (material -> texture).
    AssetMetadata* texMeta = manager.Database().FindByRawPath(canonicalTexturePath);
    HK_CHECK_MSG(texMeta != nullptr, "texture discovered");
    HK_CHECK_MSG(FileSystem::Exists(workspace / canonicalTexturePath), "texture copied under raw textures");
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
    meshMeta = manager.Database().FindByRawPath(canonicalMeshPath);
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

    const fs::path skinnedWorkspace = Paths::TempFile("skinned_gltf_ws");
    FileSystem::Remove(skinnedWorkspace);
    const fs::path skinnedModelDir = skinnedWorkspace / "data" / "raw" / "models";
    FileSystem::WriteTextFile(skinnedModelDir / "skinned.gltf", MakeSkinnedTriangleGltf());

    AssetManager skinnedManager;
    HK_CHECK_MSG(static_cast<bool>(skinnedManager.Init(AssetManager::DefaultCreateInfo(skinnedWorkspace))),
                 "skinned manager init");
    HK_CHECK_MSG(static_cast<bool>(skinnedManager.ImportAll()), "skinned import all ok");

    const fs::path skinnedMeshPath = "data/raw/meshes/skinned/SkinTri.mesh.yaml";
    const fs::path skeletonPath = "data/raw/animation/skeletons/skinned/Armature.skeleton.yaml";
    const fs::path animationPath = "data/raw/animation/clips/skinned/Wave.anim.yaml";

    AssetMetadata* skinnedModelMeta = skinnedManager.Database().FindByRawPath("data/raw/models/skinned.gltf");
    AssetMetadata* skinnedMeshMeta = skinnedManager.Database().FindByRawPath(skinnedMeshPath);
    AssetMetadata* skeletonMeta = skinnedManager.Database().FindByRawPath(skeletonPath);
    AssetMetadata* animationMeta = skinnedManager.Database().FindByRawPath(animationPath);

    HK_CHECK_MSG(skinnedModelMeta != nullptr, "skinned model discovered");
    HK_CHECK_MSG(skinnedMeshMeta != nullptr && skinnedMeshMeta->type == AssetType::Mesh,
                 "skinned mesh descriptor generated");
    HK_CHECK_MSG(skeletonMeta != nullptr && skeletonMeta->type == AssetType::Skeleton,
                 "skeleton descriptor generated");
    HK_CHECK_MSG(animationMeta != nullptr && animationMeta->type == AssetType::Animation,
                 "animation descriptor generated");
    HK_CHECK_MSG(FileSystem::Exists(skinnedWorkspace / skeletonPath), "skeleton descriptor written");
    HK_CHECK_MSG(FileSystem::Exists(skinnedWorkspace / animationPath), "animation descriptor written");

    bool meshDependsOnSkeleton = false;
    if (skinnedMeshMeta != nullptr && skeletonMeta != nullptr) {
        for (const AssetID dep : skinnedMeshMeta->dependencies) {
            meshDependsOnSkeleton = meshDependsOnSkeleton || dep == skeletonMeta->id;
        }
    }
    HK_CHECK_MSG(meshDependsOnSkeleton, "skinned mesh depends on skeleton");

    bool animationDependsOnSkeleton = false;
    if (animationMeta != nullptr && skeletonMeta != nullptr) {
        for (const AssetID dep : animationMeta->dependencies) {
            animationDependsOnSkeleton = animationDependsOnSkeleton || dep == skeletonMeta->id;
        }
    }
    HK_CHECK_MSG(animationDependsOnSkeleton, "animation depends on skeleton");

    bool modelDependsOnSkeleton = false;
    bool modelDependsOnAnimation = false;
    if (skinnedModelMeta != nullptr && skeletonMeta != nullptr && animationMeta != nullptr) {
        for (const AssetID dep : skinnedModelMeta->dependencies) {
            modelDependsOnSkeleton = modelDependsOnSkeleton || dep == skeletonMeta->id;
            modelDependsOnAnimation = modelDependsOnAnimation || dep == animationMeta->id;
        }
    }
    HK_CHECK_MSG(modelDependsOnSkeleton, "model depends on skeleton");
    HK_CHECK_MSG(modelDependsOnAnimation, "model depends on animation");

    HK_CHECK_MSG(static_cast<bool>(skinnedManager.CookAllDirty()), "skinned cook all dirty ok");

    if (skinnedMeshMeta != nullptr) {
        Result<std::shared_ptr<MeshAsset>> skinnedMesh = skinnedManager.Load<MeshAsset>(skinnedMeshMeta->id);
        HK_CHECK_MSG(static_cast<bool>(skinnedMesh), "load cooked skinned mesh");
        if (skinnedMesh) {
            bool foundWeightedVertex = false;
            for (const MeshVertex& vertex : skinnedMesh.value->vertices) {
                if (vertex.jointWeights.x > 0.7f && vertex.jointIndices.x == 0 && vertex.jointIndices.y == 1) {
                    foundWeightedVertex = true;
                }
            }
            HK_CHECK_MSG(foundWeightedVertex, "skinned mesh preserves joints and weights");
        }
    }

    if (skeletonMeta != nullptr) {
        Result<std::shared_ptr<SkeletonAsset>> skeleton = skinnedManager.Load<SkeletonAsset>(skeletonMeta->id);
        HK_CHECK_MSG(static_cast<bool>(skeleton), "load cooked skeleton");
        if (skeleton) {
            HK_CHECK_EQ(skeleton.value->bones.size(), 2u);
            HK_CHECK_EQ(skeleton.value->bones[0].name, std::string("Root"));
            HK_CHECK_EQ(skeleton.value->bones[0].parentIndex, -1);
            HK_CHECK_EQ(skeleton.value->bones[1].name, std::string("Hand"));
            HK_CHECK_EQ(skeleton.value->bones[1].parentIndex, 0);
        }
    }

    if (animationMeta != nullptr && skeletonMeta != nullptr) {
        Result<std::shared_ptr<AnimationAsset>> animation = skinnedManager.Load<AnimationAsset>(animationMeta->id);
        HK_CHECK_MSG(static_cast<bool>(animation), "load cooked animation");
        if (animation) {
            HK_CHECK_EQ(animation.value->skeletonAsset, skeletonMeta->id);
            HK_CHECK_NEAR(animation.value->durationSeconds, 1.0f, 0.001f);
            HK_CHECK_EQ(animation.value->tracks.size(), 1u);
            HK_CHECK_EQ(animation.value->tracks[0].boneIndex, 1);
            HK_CHECK_EQ(animation.value->tracks[0].translations.size(), 2u);
        }
    }

    if (skinnedModelMeta != nullptr && skeletonMeta != nullptr && animationMeta != nullptr) {
        Result<std::shared_ptr<ModelAsset>> skinnedModel = skinnedManager.Load<ModelAsset>(skinnedModelMeta->id);
        HK_CHECK_MSG(static_cast<bool>(skinnedModel), "load cooked skinned model");
        if (skinnedModel) {
            HK_CHECK_EQ(skinnedModel.value->skeletons.size(), 1u);
            HK_CHECK_EQ(skinnedModel.value->skeletons[0], skeletonMeta->id);
            HK_CHECK_EQ(skinnedModel.value->animations.size(), 1u);
            HK_CHECK_EQ(skinnedModel.value->animations[0], animationMeta->id);
        }
    }

    skinnedManager.Shutdown();
    FileSystem::Remove(skinnedWorkspace);
}
