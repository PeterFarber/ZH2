#pragma once

#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Core/Result.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace Hockey {

class SkeletonLoader {
public:
    static constexpr uint32_t kVersion = 1;

    static std::vector<std::byte> EncodeCooked(const SkeletonAsset& asset, uint64_t sourceHash);
    static Result<SkeletonAsset> DecodeCooked(const std::byte* data, size_t size);
    static Result<SkeletonAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
