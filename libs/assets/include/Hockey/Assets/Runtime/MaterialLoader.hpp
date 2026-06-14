#pragma once
#include <filesystem>
#include <string>

#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Owns the cooked material format (.material.yaml with texture references
// resolved to AssetIDs). Distinct from MaterialSerializer, which parses the
// authored raw material that references textures by path.
class MaterialLoader {
public:
    static std::string EncodeCooked(const MaterialAsset& material);
    static Result<MaterialAsset> DecodeCooked(const std::string& text);
    static Result<MaterialAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
