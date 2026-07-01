#pragma once

#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Core/Result.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace Hockey {

class AnimationLoader {
public:
    static constexpr uint32_t kVersion = 1;

    static std::vector<std::byte> EncodeCooked(const AnimationAsset& asset, uint64_t sourceHash);
    static Result<AnimationAsset> DecodeCooked(const std::byte* data, size_t size);
    static Result<AnimationAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
