#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

// Description of a shader to compile from GLSL source. Backend-agnostic and
// Vulkan-free so non-GPU tooling (and the shader compile tests) can use it.
struct ShaderDesc {
    std::filesystem::path sourcePath;
    ShaderStage stage = ShaderStage::Vertex;
    std::string entryPoint = "main";
    std::vector<std::string> defines; // "NAME" or "NAME=VALUE"
};

// Result of a successful GLSL -> SPIR-V compilation.
struct CompiledShaderData {
    std::vector<uint32_t> spirv;
    std::filesystem::path outputPath; // populated when written to disk
};

// Compiles a GLSL shader file to SPIR-V using shaderc. #include directives are
// resolved relative to the source file's directory. When binaryDir is provided,
// the SPIR-V is written to "<binaryDir>/<sourceName>.spv".
Result<CompiledShaderData> CompileShaderFile(const ShaderDesc& desc, const std::filesystem::path& binaryDir = {});

// Infers the shader stage from a file extension (.vert/.frag/.comp).
ShaderStage ShaderStageFromExtension(const std::filesystem::path& path);

// Files the renderer expects to find under data/shaders/src.
struct RequiredShaderFile {
    const char* fileName;
    bool compilable; // common.glsl is include-only
    ShaderStage stage;
};

const std::vector<RequiredShaderFile>& RequiredShaderFiles();

} // namespace Hockey
