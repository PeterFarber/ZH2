#pragma once
#include <cstddef>
#include <filesystem>
#include <vector>

#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Owns the cooked model binary format (.model.bin): the model name plus the
// AssetID lists of its generated meshes and materials.
class ModelLoader {
public:
    static constexpr uint32_t kVersion = 2;

    static std::vector<std::byte> Encode(const ModelAsset& asset, uint64_t sourceHash);
    static Result<ModelAsset> Decode(const std::byte* data, size_t size);
    static Result<ModelAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
