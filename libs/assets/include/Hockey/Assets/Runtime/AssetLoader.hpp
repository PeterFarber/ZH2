#pragma once
#include <filesystem>
#include <memory>

#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct TextureAsset;
struct MeshAsset;
struct MaterialAsset;
struct ModelAsset;
struct ShaderAsset;

// Loads cooked CPU-side assets from disk. Resolves the project-relative cooked
// path stored in metadata against the project root. The renderer turns the
// returned CPU data into GPU resources.
class AssetLoader {
public:
    explicit AssetLoader(std::filesystem::path projectRoot) : m_ProjectRoot(std::move(projectRoot)) {}

    Result<std::shared_ptr<TextureAsset>> LoadTexture(const AssetMetadata& metadata);
    Result<std::shared_ptr<MeshAsset>> LoadMesh(const AssetMetadata& metadata);
    Result<std::shared_ptr<ModelAsset>> LoadModel(const AssetMetadata& metadata);
    Result<std::shared_ptr<MaterialAsset>> LoadMaterial(const AssetMetadata& metadata);
    Result<std::shared_ptr<ShaderAsset>> LoadShader(const AssetMetadata& metadata);

private:
    std::filesystem::path CookedAbsolute(const AssetMetadata& metadata) const;

    std::filesystem::path m_ProjectRoot;
};

} // namespace Hockey
