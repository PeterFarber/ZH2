#pragma once
#include <string>

namespace Hockey {

// All asset categories tracked by the pipeline.
enum class AssetType {
    Unknown,
    Texture,
    Mesh,
    Material,
    Model,
    Shader,
    Scene,
    Prefab,
    Audio,
    Skeleton,
    Animation
};

std::string AssetTypeToString(AssetType type);
AssetType AssetTypeFromString(const std::string& value);

} // namespace Hockey
