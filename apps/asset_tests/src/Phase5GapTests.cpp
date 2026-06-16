#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/PrefabAsset.hpp"
#include "Hockey/Assets/Assets/SceneAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Assets/Runtime/TextureLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

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
    header[13] = static_cast<uint8_t>((w >> 8) & 0xFF);
    header[14] = static_cast<uint8_t>(h & 0xFF);
    header[15] = static_cast<uint8_t>((h >> 8) & 0xFF);
    header[16] = 32;
    header[17] = 0x28;
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
        const uint8_t bgra[4] = {rgba[i + 2], rgba[i + 1], rgba[i + 0], rgba[i + 3]};
        out.write(reinterpret_cast<const char*>(bgra), 4);
    }
}

// A flat quad in the XY plane (normal +Z) with UVs rotated so the U axis maps
// to object +Y. A correctly generated tangent therefore points along +Y, which
// is clearly distinct from the default (1,0,0,1), so the test verifies that
// tangents were actually synthesized (the primitive omits TANGENT).
std::string MakeQuadGltfNoTangents() {
    std::vector<uint8_t> buffer;
    // POSITION (4 x vec3)
    const float positions[4][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
    for (const auto& p : positions) {
        AppendFloat(buffer, p[0]);
        AppendFloat(buffer, p[1]);
        AppendFloat(buffer, p[2]);
    }
    // NORMAL (4 x vec3) all +Z
    for (int i = 0; i < 4; ++i) {
        AppendFloat(buffer, 0.0f);
        AppendFloat(buffer, 0.0f);
        AppendFloat(buffer, 1.0f);
    }
    // TEXCOORD_0 (4 x vec2): U follows object +Y, V follows object +X.
    const float uvs[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    for (const auto& uv : uvs) {
        AppendFloat(buffer, uv[0]);
        AppendFloat(buffer, uv[1]);
    }
    // INDICES (6 x u16): two triangles.
    const uint16_t indices[6] = {0, 1, 2, 2, 1, 3};
    for (const uint16_t idx : indices) {
        AppendU16(buffer, idx);
    }

    const std::string b64 = Base64Encode(buffer);
    return std::string(R"({
  "asset": {"version": "2.0"},
  "meshes": [{"name": "Quad", "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3}]}],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3", "min": [0,0,0], "max": [1,1,0]},
    {"bufferView": 1, "componentType": 5126, "count": 4, "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC2"},
    {"bufferView": 3, "componentType": 5123, "count": 6, "type": "SCALAR"}
  ],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 48},
    {"buffer": 0, "byteOffset": 48, "byteLength": 48},
    {"buffer": 0, "byteOffset": 96, "byteLength": 32},
    {"buffer": 0, "byteOffset": 128, "byteLength": 12}
  ],
  "buffers": [{"byteLength": 140, "uri": "data:application/octet-stream;base64,)") +
           b64 + R"("}]
})";
}

} // namespace

// --- Texture maxSize clamp (TextureLoader::LoadRaw) ---------------------------
void RunTextureMaxSizeTests() {
    HockeyTest::BeginSuite("TextureMaxSizeTests");

    const fs::path workspace = Paths::TempFile("texmax_ws");
    FileSystem::Remove(workspace);
    const fs::path tex = workspace / "big.tga";

    // 8x8 solid texture.
    std::vector<uint8_t> px(8 * 8 * 4, 0);
    for (size_t i = 0; i < px.size(); i += 4) {
        px[i + 0] = 200;
        px[i + 1] = 100;
        px[i + 2] = 50;
        px[i + 3] = 255;
    }
    WriteTga(tex, 8, 8, px);

    TextureImportSettings clamp;
    clamp.maxSize = 2;
    Result<TextureAsset> clamped = TextureLoader::LoadRaw(tex, clamp);
    HK_CHECK_MSG(static_cast<bool>(clamped), "load raw with maxSize clamp");
    if (clamped) {
        HK_CHECK_EQ(static_cast<int>(clamped.value.width), 2);
        HK_CHECK_EQ(static_cast<int>(clamped.value.height), 2);
    }

    TextureImportSettings full;
    full.maxSize = 4096;
    Result<TextureAsset> unclamped = TextureLoader::LoadRaw(tex, full);
    HK_CHECK_MSG(static_cast<bool>(unclamped), "load raw without clamp");
    if (unclamped) {
        HK_CHECK_EQ(static_cast<int>(unclamped.value.width), 8);
        HK_CHECK_EQ(static_cast<int>(unclamped.value.height), 8);
    }

    FileSystem::Remove(workspace);
}

// --- glTF tangent generation when TANGENT is omitted --------------------------
void RunTangentGenerationTests() {
    HockeyTest::BeginSuite("TangentGenerationTests");

    const fs::path workspace = Paths::TempFile("tangent_ws");
    FileSystem::Remove(workspace);
    const fs::path modelDir = workspace / "data" / "raw" / "models";
    FileSystem::WriteTextFile(modelDir / "quad.gltf", MakeQuadGltfNoTangents());

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");

    AssetMetadata* meshMeta = manager.Database().FindByRawPath("data/raw/models/quad.gltf#mesh0");
    HK_CHECK_MSG(meshMeta != nullptr, "quad mesh sub-asset generated");
    if (meshMeta == nullptr) {
        manager.Shutdown();
        FileSystem::Remove(workspace);
        return;
    }

    Result<std::shared_ptr<MeshAsset>> mesh = manager.Load<MeshAsset>(meshMeta->id);
    HK_CHECK_MSG(static_cast<bool>(mesh), "load cooked quad mesh");
    if (mesh) {
        HK_CHECK_MSG(!mesh.value->vertices.empty(), "quad has vertices");
        bool allGenerated = true;
        bool anyNonDefault = false;
        for (const MeshVertex& v : mesh.value->vertices) {
            const glm::vec3 t(v.tangent.x, v.tangent.y, v.tangent.z);
            const float len = glm::length(t);
            // Synthesized tangent should point along +Y (UVs rotate U onto +Y).
            if (std::abs(len - 1.0f) > 0.05f || std::abs(t.y) < 0.9f || std::abs(t.x) > 0.1f ||
                std::abs(t.z) > 0.1f) {
                allGenerated = false;
            }
            if (std::abs(t.x - 1.0f) > 0.01f || std::abs(t.y) > 0.01f) {
                anyNonDefault = true;
            }
            HK_CHECK_MSG(std::abs(std::abs(v.tangent.w) - 1.0f) < 0.01f, "tangent w is unit handedness");
        }
        HK_CHECK_MSG(anyNonDefault, "tangents differ from default (1,0,0,1)");
        HK_CHECK_MSG(allGenerated, "tangents synthesized along expected axis");
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}

// --- Scene/Prefab runtime loaders + hot-reload invalidation -------------------
void RunSceneAssetTests() {
    HockeyTest::BeginSuite("SceneAssetTests");

    const fs::path workspace = Paths::TempFile("sceneasset_ws");
    FileSystem::Remove(workspace);
    const fs::path raw = workspace / "data" / "raw";

    const std::vector<uint8_t> px = {180, 180, 180, 255, 180, 180, 180, 255,
                                     180, 180, 180, 255, 180, 180, 180, 255};
    WriteTga(raw / "textures" / "ice.tga", 2, 2, px);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import textures");
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook textures");

    AssetMetadata* texMeta = manager.Database().FindByRawPath("data/raw/textures/ice.tga");
    HK_CHECK_MSG(texMeta != nullptr, "texture discovered");
    if (texMeta == nullptr) {
        manager.Shutdown();
        FileSystem::Remove(workspace);
        return;
    }
    const std::string texId = std::to_string(texMeta->id.Value());

    // Scene + prefab YAML referencing the texture id as a dependency.
    const std::string sceneYaml = "Scene:\n  Name: TestLevel\n  Entities:\n    - BaseColor: " + texId + "\n";
    const std::string prefabYaml = "Prefab:\n  Name: Puck\n  Components:\n    Material: " + texId + "\n";
    FileSystem::WriteTextFile(raw / "scenes" / "level.scene.yaml", sceneYaml);
    FileSystem::WriteTextFile(raw / "prefabs" / "puck.prefab.yaml", prefabYaml);

    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "rediscover scene/prefab");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import scene/prefab");
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook scene/prefab");

    AssetMetadata* sceneMeta = manager.Database().FindByRawPath("data/raw/scenes/level.scene.yaml");
    AssetMetadata* prefabMeta = manager.Database().FindByRawPath("data/raw/prefabs/puck.prefab.yaml");
    HK_CHECK_MSG(sceneMeta != nullptr && sceneMeta->type == AssetType::Scene, "scene asset present");
    HK_CHECK_MSG(prefabMeta != nullptr && prefabMeta->type == AssetType::Prefab, "prefab asset present");
    if (sceneMeta == nullptr || prefabMeta == nullptr) {
        manager.Shutdown();
        FileSystem::Remove(workspace);
        return;
    }

    // Load<SceneAsset>.
    Result<std::shared_ptr<SceneAsset>> scene = manager.Load<SceneAsset>(sceneMeta->id);
    HK_CHECK_MSG(static_cast<bool>(scene), "load scene asset");
    if (scene) {
        HK_CHECK_MSG(!scene.value->name.empty(), "scene name populated");
        HK_CHECK_MSG(scene.value->sourcePath == sceneMeta->rawPath, "scene source path");
        bool dependsOnTex = false;
        for (const AssetID dep : scene.value->dependencies) {
            dependsOnTex = dependsOnTex || dep == texMeta->id;
        }
        HK_CHECK_MSG(dependsOnTex, "scene depends on texture");
    }

    // Load<PrefabAsset>.
    Result<std::shared_ptr<PrefabAsset>> prefab = manager.Load<PrefabAsset>(prefabMeta->id);
    HK_CHECK_MSG(static_cast<bool>(prefab), "load prefab asset");
    if (prefab) {
        bool dependsOnTex = false;
        for (const AssetID dep : prefab.value->dependencies) {
            dependsOnTex = dependsOnTex || dep == texMeta->id;
        }
        HK_CHECK_MSG(dependsOnTex, "prefab depends on texture");
    }

    // Type mismatches are rejected.
    HK_CHECK_MSG(!manager.Load<SceneAsset>(prefabMeta->id), "scene loader rejects prefab id");
    HK_CHECK_MSG(!manager.Load<PrefabAsset>(sceneMeta->id), "prefab loader rejects scene id");
    HK_CHECK_MSG(!manager.Load<SceneAsset>(texMeta->id), "scene loader rejects texture id");

    // --- Hot-reload invalidates the cached scene and emits Reloaded ----------
    HK_CHECK_MSG(manager.Registry().IsLoaded(sceneMeta->id), "scene cached after load");
    manager.StartWatching();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const fs::path sceneAbs = raw / "scenes" / "level.scene.yaml";
    FileSystem::WriteTextFile(sceneAbs, sceneYaml + "# touched\n");
    fs::last_write_time(sceneAbs, fs::file_time_type::clock::now());

    const int changed = manager.PollHotReload(/*autoImport=*/true, /*autoCookDirty=*/true);
    HK_CHECK_MSG(changed > 0, "hot reload detects scene change");

    bool sawReloaded = false;
    for (const AssetEvent& event : manager.PollEvents()) {
        if (event.type == AssetEventType::Reloaded && event.id == sceneMeta->id) {
            sawReloaded = true;
        }
    }
    HK_CHECK_MSG(sawReloaded, "reloaded event emitted for scene");
    HK_CHECK_MSG(!manager.Registry().IsLoaded(sceneMeta->id), "scene evicted from registry on reload");

    // Re-loading after invalidation succeeds.
    Result<std::shared_ptr<SceneAsset>> reloaded = manager.Load<SceneAsset>(sceneMeta->id);
    HK_CHECK_MSG(static_cast<bool>(reloaded), "scene reloads after invalidation");

    manager.StopWatching();

    // --- DeleteMetadata drops the sidecar + db record, keeps the raw file ----
    AssetMetadata* texAgain = manager.Database().FindByRawPath("data/raw/textures/ice.tga");
    HK_CHECK_MSG(texAgain != nullptr, "texture still tracked before delete-metadata");
    if (texAgain != nullptr) {
        const AssetID texAssetId = texAgain->id;
        const fs::path sidecarAbs = workspace / texAgain->metadataPath;
        HK_CHECK_MSG(static_cast<bool>(manager.DeleteMetadata(texAssetId)), "delete metadata ok");
        HK_CHECK_MSG(manager.Database().Find(texAssetId) == nullptr, "db record removed");
        HK_CHECK_MSG(manager.Database().FindByRawPath("data/raw/textures/ice.tga") == nullptr,
                     "path index cleared after delete-metadata");
        HK_CHECK_MSG(!FileSystem::Exists(sidecarAbs), "sidecar file deleted");
        HK_CHECK_MSG(FileSystem::Exists(raw / "textures" / "ice.tga"), "raw file left intact");
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
