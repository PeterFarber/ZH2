#pragma once
#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Typed, lightweight reference to an asset. Stores only an AssetID; the actual
// loaded object is fetched from the AssetRegistry/AssetManager. The template
// parameter is a compile-time tag for type safety and never needs to be a
// complete type for the handle itself.
template <typename T> class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(AssetID id) : m_ID(id) {}

    AssetID ID() const { return m_ID; }
    bool IsValid() const { return m_ID.IsValid(); }

    bool operator==(const AssetHandle& other) const = default;
    bool operator!=(const AssetHandle& other) const = default;

private:
    AssetID m_ID;
};

// Forward declarations so the handle aliases can be used widely without
// pulling in every asset definition.
struct TextureAsset;
struct MeshAsset;
struct MaterialAsset;
struct ModelAsset;
struct ShaderAsset;
struct SceneAsset;
struct PrefabAsset;

using TextureAssetHandle = AssetHandle<TextureAsset>;
using MeshAssetHandle = AssetHandle<MeshAsset>;
using MaterialAssetHandle = AssetHandle<MaterialAsset>;
using ModelAssetHandle = AssetHandle<ModelAsset>;
using ShaderAssetHandle = AssetHandle<ShaderAsset>;
using SceneAssetHandle = AssetHandle<SceneAsset>;
using PrefabAssetHandle = AssetHandle<PrefabAsset>;

} // namespace Hockey
