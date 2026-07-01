#include "Hockey/Assets/Runtime/AssetLoader.hpp"

#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Assets/Runtime/AudioLoader.hpp"
#include "Hockey/Assets/Runtime/AnimationLoader.hpp"
#include "Hockey/Assets/Runtime/MaterialLoader.hpp"
#include "Hockey/Assets/Runtime/MeshLoader.hpp"
#include "Hockey/Assets/Runtime/ModelLoader.hpp"
#include "Hockey/Assets/Runtime/ShaderLoader.hpp"
#include "Hockey/Assets/Runtime/SkeletonLoader.hpp"
#include "Hockey/Assets/Runtime/TextureLoader.hpp"

namespace Hockey {
namespace fs = std::filesystem;

fs::path AssetLoader::CookedAbsolute(const AssetMetadata& metadata) const {
    if (metadata.cookedPath.is_absolute()) {
        return metadata.cookedPath;
    }
    return m_ProjectRoot / metadata.cookedPath;
}

Result<std::shared_ptr<TextureAsset>> AssetLoader::LoadTexture(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<TextureAsset>>::Fail("texture has no cooked path");
    }
    Result<TextureAsset> loaded = TextureLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<TextureAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<TextureAsset>>::Ok(std::make_shared<TextureAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<MeshAsset>> AssetLoader::LoadMesh(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<MeshAsset>>::Fail("mesh has no cooked path");
    }
    Result<MeshAsset> loaded = MeshLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<MeshAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<MeshAsset>>::Ok(std::make_shared<MeshAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<ModelAsset>> AssetLoader::LoadModel(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<ModelAsset>>::Fail("model has no cooked path");
    }
    Result<ModelAsset> loaded = ModelLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<ModelAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<ModelAsset>>::Ok(std::make_shared<ModelAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<MaterialAsset>> AssetLoader::LoadMaterial(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<MaterialAsset>>::Fail("material has no cooked path");
    }
    Result<MaterialAsset> loaded = MaterialLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<MaterialAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<MaterialAsset>>::Ok(std::make_shared<MaterialAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<ShaderAsset>> AssetLoader::LoadShader(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<ShaderAsset>>::Fail("shader has no cooked path");
    }
    const ShaderAssetStage stage = ShaderStageFromExtension(metadata.rawPath.extension().string());
    Result<ShaderAsset> loaded = ShaderLoader::LoadCooked(CookedAbsolute(metadata), stage);
    if (!loaded) {
        return Result<std::shared_ptr<ShaderAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<ShaderAsset>>::Ok(std::make_shared<ShaderAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<AudioAsset>> AssetLoader::LoadAudio(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<AudioAsset>>::Fail("audio has no cooked path");
    }
    Result<AudioAsset> loaded = AudioLoader::LoadCooked(CookedAbsolute(metadata), metadata.id);
    if (!loaded) {
        return Result<std::shared_ptr<AudioAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<AudioAsset>>::Ok(std::make_shared<AudioAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<SkeletonAsset>> AssetLoader::LoadSkeleton(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<SkeletonAsset>>::Fail("skeleton has no cooked path");
    }
    Result<SkeletonAsset> loaded = SkeletonLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<SkeletonAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<SkeletonAsset>>::Ok(std::make_shared<SkeletonAsset>(std::move(loaded.value)));
}

Result<std::shared_ptr<AnimationAsset>> AssetLoader::LoadAnimation(const AssetMetadata& metadata) {
    if (metadata.cookedPath.empty()) {
        return Result<std::shared_ptr<AnimationAsset>>::Fail("animation has no cooked path");
    }
    Result<AnimationAsset> loaded = AnimationLoader::LoadCooked(CookedAbsolute(metadata));
    if (!loaded) {
        return Result<std::shared_ptr<AnimationAsset>>::Fail(loaded.error);
    }
    return Result<std::shared_ptr<AnimationAsset>>::Ok(std::make_shared<AnimationAsset>(std::move(loaded.value)));
}

} // namespace Hockey
