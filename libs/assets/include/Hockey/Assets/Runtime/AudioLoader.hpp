#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

class AudioLoader {
public:
    static constexpr uint32_t kVersion = 1;

    static std::vector<std::byte> EncodeCooked(const AudioAsset& asset, uint64_t sourceHash);
    static Result<AudioAsset> DecodeCooked(const std::byte* data, size_t size, AssetID id = {});
    static Result<AudioAsset> LoadCooked(const std::filesystem::path& cookedPath, AssetID id = {});
};

} // namespace Hockey
