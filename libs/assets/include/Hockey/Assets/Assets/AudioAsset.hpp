#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

struct AudioAsset {
    AssetID id;
    std::string debugName;
    std::string sourceExtension;
    std::vector<uint8_t> encodedBytes;
};

} // namespace Hockey
