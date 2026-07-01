#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct AudioAsset;

struct AudioClip {
    AssetID id;
    std::string name;
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 2;
    std::uint64_t frameCount = 0;
    float durationSeconds = 0.0f;
    std::vector<float> samples;

    static Result<AudioClip> Decode(const AudioAsset& asset);
};

} // namespace Hockey
