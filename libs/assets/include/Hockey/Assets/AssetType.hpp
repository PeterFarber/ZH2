#pragma once
#include <string>

namespace Hockey {

// All asset categories tracked by the pipeline. Audio and Animation are
// future-facing placeholders in Phase 5: they are recognized as enum values
// but have no importer/cooker yet.
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
    Animation
};

std::string AssetTypeToString(AssetType type);
AssetType AssetTypeFromString(const std::string& value);

} // namespace Hockey
