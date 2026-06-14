#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Named separately from the renderer's ShaderStage so hockey_assets stays
// independent of the renderer. The renderer maps this to its own stage enum.
enum class ShaderAssetStage { Vertex, Fragment, Compute, Unknown };

// Runtime, CPU-side shader: compiled SPIR-V plus the stage it targets. The
// renderer creates a VkShaderModule from `spirv`.
struct ShaderAsset {
    AssetID id;
    ShaderAssetStage stage = ShaderAssetStage::Unknown;
    std::string entryPoint = "main";
    std::vector<uint32_t> spirv;
};

std::string ShaderAssetStageToString(ShaderAssetStage stage);
ShaderAssetStage ShaderAssetStageFromString(const std::string& value);
// Infers a stage from a shader source file extension (.vert/.frag/.comp/.glsl).
ShaderAssetStage ShaderStageFromExtension(const std::string& extension);

} // namespace Hockey
