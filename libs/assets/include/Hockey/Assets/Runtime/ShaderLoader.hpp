#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Reads/writes cooked SPIR-V (.spv) as raw 32-bit words so the file remains a
// valid standalone SPIR-V module. The stage is supplied by the caller (derived
// from the source extension) since a bare .spv does not record it.
class ShaderLoader {
public:
    static Status WriteSpv(const std::filesystem::path& cookedPath,
                           const std::vector<uint32_t>& spirv);

    static Result<ShaderAsset> LoadCooked(const std::filesystem::path& cookedPath,
                                          ShaderAssetStage stage);
    static Result<ShaderAsset> LoadCooked(const std::filesystem::path& cookedPath);
};

} // namespace Hockey
