#pragma once
#include <cstddef>
#include <filesystem>
#include <vector>

#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Owns the cooked mesh binary format (.mesh.bin): interleaved vertices, 32-bit
// indices, submeshes, and bounds.
class MeshLoader {
public:
    static constexpr uint32_t kVersion = 2;

    static std::vector<std::byte> Encode(const MeshAsset& asset, uint64_t sourceHash);
    static Result<MeshAsset> Decode(const std::byte* data, size_t size);
    static Result<MeshAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
