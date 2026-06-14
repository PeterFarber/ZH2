#pragma once
#include <filesystem>
#include <string>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Importer.hpp"

namespace Hockey {

class AssetDatabase;

// Imports .gltf/.glb models. The model file is the primary asset (AssetType::Model);
// each glTF mesh and material is registered as a generated sub-asset with a
// synthetic raw path of the form "<model>#mesh<i>" / "<model>#material<i>" so the
// mesh/material cookers can re-derive them from the source glTF.
class GltfImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override {
        return AssetType::Model;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    ImportResult Import(const ImportContext& context) override;

    // Synthetic raw paths for generated sub-assets. `modelRawPath` is the
    // project-relative path to the .gltf/.glb file.
    static std::string MeshSubAssetPath(const std::filesystem::path& modelRawPath, size_t meshIndex);
    static std::string MaterialSubAssetPath(const std::filesystem::path& modelRawPath, size_t materialIndex);

    // True when a raw path refers to a glTF-generated sub-asset.
    static bool IsSubAsset(const std::filesystem::path& rawPath);
    // Splits "<model>#mesh3" into ("<model>", 3, true). `outIsMesh` distinguishes
    // mesh ("#mesh") from material ("#material") sub-assets.
    static bool ParseSubAsset(const std::filesystem::path& rawPath, std::filesystem::path& outModelRawPath,
                              size_t& outIndex, bool& outIsMesh);

    // Resolves a glTF-authored texture URI (relative to the glTF directory) to a
    // project-relative path. Empty input -> empty output.
    static std::filesystem::path TextureProjectPath(const std::filesystem::path& projectRoot,
                                                    const std::filesystem::path& modelRawPath, const std::string& uri);
};

} // namespace Hockey
