#include "Test.hpp"

#include <fstream>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Renderer/Shader.hpp"

using namespace Hockey;

// Compiles every required GLSL shader to SPIR-V on the CPU (no GPU/device
// needed) and validates error reporting and on-disk output.
void RunShaderCompileTests() {
    HockeyTest::BeginSuite("ShaderCompileTests");

    const std::filesystem::path srcDir = Paths::Get().data / "shaders" / "src";
    const std::filesystem::path binDir = Paths::Get().temp / "shaders_test_bin";

    // 1. All required shader files exist.
    for (const RequiredShaderFile& shader : RequiredShaderFiles()) {
        const std::filesystem::path path = srcDir / shader.fileName;
        HK_CHECK_MSG(std::filesystem::exists(path), shader.fileName);
    }

    // 2. All compilable shaders compile to SPIR-V and write valid output.
    for (const RequiredShaderFile& shader : RequiredShaderFiles()) {
        if (!shader.compilable) {
            continue;
        }
        ShaderDesc desc;
        desc.sourcePath = srcDir / shader.fileName;
        desc.stage = shader.stage;
        Result<CompiledShaderData> compiled = CompileShaderFile(desc, binDir);
        HK_CHECK_MSG(static_cast<bool>(compiled), shader.fileName);
        if (compiled) {
            HK_CHECK(!compiled.value.spirv.empty());
            // SPIR-V magic number.
            HK_CHECK(compiled.value.spirv.front() == 0x07230203u);
            // 3. Compiled output path exists.
            HK_CHECK(std::filesystem::exists(compiled.value.outputPath));
        }
    }

    // 4. Compile failure returns a useful (non-empty) error.
    const std::filesystem::path badPath = binDir / "broken.frag";
    {
        std::filesystem::create_directories(binDir);
        std::ofstream bad(badPath);
        bad << "#version 450\nvoid main() { this is not valid glsl }\n";
    }
    {
        ShaderDesc desc;
        desc.sourcePath = badPath;
        desc.stage = ShaderStage::Fragment;
        Result<CompiledShaderData> compiled = CompileShaderFile(desc, {});
        HK_CHECK(!compiled);
        HK_CHECK(!compiled.error.empty());
    }

    // Missing file also fails cleanly.
    {
        ShaderDesc desc;
        desc.sourcePath = srcDir / "does_not_exist.vert";
        desc.stage = ShaderStage::Vertex;
        Result<CompiledShaderData> compiled = CompileShaderFile(desc, {});
        HK_CHECK(!compiled);
    }

    std::error_code ec;
    std::filesystem::remove_all(binDir, ec);
}
