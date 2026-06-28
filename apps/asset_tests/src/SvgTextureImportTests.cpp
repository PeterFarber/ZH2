#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cstddef>
#include <cstdint>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

const char* kCenterLogoSvg = R"(<svg xmlns="http://www.w3.org/2000/svg" width="64" height="32" viewBox="0 0 64 32">
  <rect x="0" y="0" width="64" height="32" fill="#ff0000" fill-opacity="0.75"/>
  <circle cx="32" cy="16" r="8" fill="#ffffff" fill-opacity="1"/>
</svg>
)";

const char* kSvgDecalMaterial = R"(Material:
  Name: Center Ice Logo Decal
  Type: PBR
  BaseColor: [1, 1, 1, 1]
  Metallic: 0
  Roughness: 0.65
  NormalStrength: 1
  OcclusionStrength: 1
  EmissiveColor: [0, 0, 0]
  EmissiveStrength: 0
  AlphaMode: Blend
  AlphaCutoff: 0.5
  Tiling: [1, 1]
  Offset: [0, 0]
  Textures:
    BaseColor: data/raw/textures/rink_decals/center_logo.svg
)";

uint8_t ByteAt(const TextureAsset& texture, size_t offset) {
    return static_cast<uint8_t>(texture.data[offset]);
}

} // namespace

void RunSvgTextureImportTests() {
    HockeyTest::BeginSuite("SvgTextureImportTests");

    const fs::path workspace = Paths::TempFile("svg_texture_ws");
    FileSystem::Remove(workspace);

    const fs::path svgPath = workspace / "data" / "raw" / "textures" / "rink_decals" / "center_logo.svg";
    const fs::path materialPath = workspace / "data" / "raw" / "materials" / "rink_decals" /
                                  "center_logo_decal.material.yaml";
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(svgPath, kCenterLogoSvg)), "svg source written");
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(materialPath, kSvgDecalMaterial)),
                 "material source written");

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "discover svg ok");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import svg ok");

    AssetMetadata* svgMeta = manager.Database().FindByRawPath("data/raw/textures/rink_decals/center_logo.svg");
    HK_CHECK_MSG(svgMeta != nullptr, "svg metadata created");
    HK_CHECK_MSG(svgMeta != nullptr && svgMeta->type == AssetType::Texture, "svg is imported as texture");

    AssetMetadata* materialMeta =
        manager.Database().FindByRawPath("data/raw/materials/rink_decals/center_logo_decal.material.yaml");
    HK_CHECK_MSG(materialMeta != nullptr, "svg decal material metadata created");
    HK_CHECK_MSG(materialMeta != nullptr && materialMeta->type == AssetType::Material, "svg decal material is material");

    bool materialDependsOnSvg = false;
    if (materialMeta != nullptr && svgMeta != nullptr) {
        for (const AssetID dep : materialMeta->dependencies) {
            materialDependsOnSvg = materialDependsOnSvg || dep == svgMeta->id;
        }
    }
    HK_CHECK_MSG(materialDependsOnSvg, "material depends on svg texture");

    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook svg texture and material ok");
    svgMeta = manager.Database().FindByRawPath("data/raw/textures/rink_decals/center_logo.svg");
    HK_CHECK_MSG(svgMeta != nullptr && svgMeta->cooked, "svg texture cooked");
    HK_CHECK_MSG(svgMeta != nullptr && FileSystem::Exists(workspace / svgMeta->cookedPath),
                 "svg cooked file exists");

    Result<std::shared_ptr<TextureAsset>> texture = manager.Load<TextureAsset>(svgMeta != nullptr ? svgMeta->id : AssetID{});
    HK_CHECK_MSG(static_cast<bool>(texture), "load cooked svg texture");
    if (texture) {
        HK_CHECK_EQ(texture.value->width, 64u);
        HK_CHECK_EQ(texture.value->height, 32u);
        HK_CHECK_MSG(texture.value->mipLevels >= 6u, "svg texture has mip chain");
        HK_CHECK_MSG(texture.value->colorSpace == TextureColorSpace::SRGB, "svg texture is sRGB");
        HK_CHECK_MSG(texture.value->semantic == TextureSemantic::BaseColor, "svg texture is base color semantic");
        HK_CHECK_MSG(texture.value->data.size() >= 64u * 32u * 4u, "svg texture has base mip data");
        HK_CHECK_MSG(ByteAt(*texture.value, 0) > 200u, "svg first pixel red channel rasterized");
        HK_CHECK_MSG(ByteAt(*texture.value, 3) > 180u, "svg first pixel alpha rasterized");
    }

    materialMeta = manager.Database().FindByRawPath("data/raw/materials/rink_decals/center_logo_decal.material.yaml");
    Result<std::shared_ptr<MaterialAsset>> material =
        manager.Load<MaterialAsset>(materialMeta != nullptr ? materialMeta->id : AssetID{});
    HK_CHECK_MSG(static_cast<bool>(material), "load cooked svg decal material");
    if (material && svgMeta != nullptr) {
        HK_CHECK_MSG(material.value->name == "Center Ice Logo Decal", "svg decal material name loaded");
        HK_CHECK_MSG(material.value->alphaMode == "Blend", "svg decal material alpha mode preserved");
        HK_CHECK_MSG(material.value->baseColorTexture == svgMeta->id, "svg texture id assigned to material base color");
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
