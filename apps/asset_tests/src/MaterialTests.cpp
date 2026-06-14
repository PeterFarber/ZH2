#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

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

const char* kMaterialYaml = R"(Material:
  Name: Ice
  Type: PBR
  BaseColor: [0.75, 0.9, 1.0, 1.0]
  Metallic: 0.0
  Roughness: 0.18
  NormalStrength: 1.0
  AlphaMode: Opaque
  Textures:
    BaseColor: data/raw/textures/ice_basecolor.tga
    Normal: data/raw/textures/ice_normal.tga
)";

} // namespace

void RunMaterialTests() {
    HockeyTest::BeginSuite("MaterialTests");

    // --- Parse raw material YAML directly. ---
    Result<MaterialSource> parsed = MaterialSerializer::Deserialize(kMaterialYaml);
    HK_CHECK_MSG(static_cast<bool>(parsed), "material yaml parses");
    if (parsed) {
        HK_CHECK_MSG(parsed.value.name == "Ice", "name parsed");
        HK_CHECK_NEAR(parsed.value.baseColor.r, 0.75f, 0.0001f);
        HK_CHECK_NEAR(parsed.value.baseColor.b, 1.0f, 0.0001f);
        HK_CHECK_NEAR(parsed.value.roughness, 0.18f, 0.0001f);
        HK_CHECK_NEAR(parsed.value.metallic, 0.0f, 0.0001f);
        HK_CHECK_MSG(parsed.value.baseColorTexture == "data/raw/textures/ice_basecolor.tga",
                     "base color texture path parsed");
        HK_CHECK_MSG(parsed.value.normalTexture == "data/raw/textures/ice_normal.tga", "normal texture path parsed");
        HK_CHECK_MSG(MaterialSerializer::TexturePaths(parsed.value).size() == 2, "two texture references detected");
    }

    // --- Serialize/parse round-trip. ---
    if (parsed) {
        const std::string text = MaterialSerializer::Serialize(parsed.value);
        Result<MaterialSource> reparsed = MaterialSerializer::Deserialize(text);
        HK_CHECK_MSG(static_cast<bool>(reparsed), "round-trip parses");
        HK_CHECK_MSG(reparsed && reparsed.value.name == "Ice", "round-trip name");
        HK_CHECK_MSG(reparsed && reparsed.value.alphaMode == "Opaque", "round-trip alpha mode");
        HK_CHECK_MSG(reparsed && reparsed.value.normalTexture == parsed.value.normalTexture, "round-trip normal path");
    }

    // --- Full import/cook/load against a workspace. ---
    const fs::path workspace = Paths::TempFile("material_ws");
    FileSystem::Remove(workspace);

    const std::vector<uint8_t> pixels = {191, 230, 255, 255, 191, 230, 255, 255,
                                         191, 230, 255, 255, 191, 230, 255, 255};
    WriteTga(workspace / "data" / "raw" / "textures" / "ice_basecolor.tga", 2, 2, pixels);
    WriteTga(workspace / "data" / "raw" / "textures" / "ice_normal.tga", 2, 2, pixels);
    FileSystem::WriteTextFile(workspace / "data" / "raw" / "materials" / "ice.material.yaml", kMaterialYaml);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");

    AssetMetadata* matMeta = manager.Database().FindByRawPath("data/raw/materials/ice.material.yaml");
    HK_CHECK_MSG(matMeta != nullptr, "material discovered");
    HK_CHECK_MSG(matMeta != nullptr && matMeta->type == AssetType::Material, "type is material");

    AssetMetadata* baseTexMeta = manager.Database().FindByRawPath("data/raw/textures/ice_basecolor.tga");
    AssetMetadata* normalTexMeta = manager.Database().FindByRawPath("data/raw/textures/ice_normal.tga");
    HK_CHECK_MSG(baseTexMeta != nullptr && normalTexMeta != nullptr, "textures discovered");

    // Dependencies recorded at import time.
    bool dependsOnBase = false;
    bool dependsOnNormal = false;
    if (matMeta != nullptr && baseTexMeta != nullptr && normalTexMeta != nullptr) {
        for (const AssetID dep : matMeta->dependencies) {
            dependsOnBase = dependsOnBase || dep == baseTexMeta->id;
            dependsOnNormal = dependsOnNormal || dep == normalTexMeta->id;
        }
    }
    HK_CHECK_MSG(dependsOnBase, "material depends on base color texture");
    HK_CHECK_MSG(dependsOnNormal, "material depends on normal texture");

    // Cook and load the cooked material.
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");
    matMeta = manager.Database().FindByRawPath("data/raw/materials/ice.material.yaml");
    HK_CHECK_MSG(matMeta != nullptr && matMeta->cooked, "material cooked");

    const AssetID matId = matMeta->id;
    Result<std::shared_ptr<MaterialAsset>> loaded = manager.Load<MaterialAsset>(matId);
    HK_CHECK_MSG(static_cast<bool>(loaded), "load cooked material");
    if (loaded) {
        HK_CHECK_MSG(loaded.value->name == "Ice", "cooked name");
        HK_CHECK_NEAR(loaded.value->roughness, 0.18f, 0.0001f);
        HK_CHECK_MSG(baseTexMeta != nullptr && loaded.value->baseColorTexture == baseTexMeta->id,
                     "cooked base color texture id resolved");
        HK_CHECK_MSG(normalTexMeta != nullptr && loaded.value->normalTexture == normalTexMeta->id,
                     "cooked normal texture id resolved");
        HK_CHECK_MSG(!loaded.value->metallicRoughnessTexture.IsValid(), "unreferenced slot is invalid id");
    }

    // Validation passes when every dependency exists.
    HK_CHECK_MSG(static_cast<bool>(manager.ValidateReferences()), "validation passes");

    // --- Missing texture dependency is reported. ---
    const char* kBadMaterial = R"(Material:
  Name: Broken
  BaseColor: [1, 1, 1, 1]
  Textures:
    BaseColor: data/raw/textures/does_not_exist.png
)";
    FileSystem::WriteTextFile(workspace / "data" / "raw" / "materials" / "broken.material.yaml", kBadMaterial);
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import broken material");
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "broken material still cooks");

    std::vector<AssetValidationIssue> issues;
    const Status validation = manager.ValidateReferences(&issues);
    HK_CHECK_MSG(!validation, "validation fails with missing dependency");
    bool reportedMissing = false;
    for (const AssetValidationIssue& issue : issues) {
        if (issue.error && issue.message.find("missing") != std::string::npos) {
            reportedMissing = true;
        }
    }
    HK_CHECK_MSG(reportedMissing, "missing dependency reported as error");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
