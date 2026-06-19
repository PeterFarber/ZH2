#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Renderer/Shader.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace {

void PrintUsage() {
    std::printf(
        "HockeyShaderTool - compile renderer GLSL shaders\n\n"
        "Usage: HockeyShaderTool [options]\n\n"
        "Options:\n"
        "  --root <dir>     Project root directory (default: .)\n"
        "  --src <dir>      Shader source directory (default: data/shaders/src)\n"
        "  --bin <dir>      Shader binary directory (default: data/shaders/bin)\n"
        "  --help, -h       Print this message and exit\n");
}

} // namespace

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    if (commandLine.Has("--help") || commandLine.Has("-h")) {
        PrintUsage();
        return 0;
    }

    const std::string root = commandLine.GetString("--root", ".");
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    Hockey::Log::Init(Hockey::Paths::LogFile("shader_tool.log"));

    const std::filesystem::path shaderSrc =
        commandLine.Has("--src") ? std::filesystem::path(commandLine.GetString("--src", "")) :
                                   Hockey::Paths::Get().root / "data/shaders/src";
    const std::filesystem::path shaderBin =
        commandLine.Has("--bin") ? std::filesystem::path(commandLine.GetString("--bin", "")) :
                                   Hockey::Paths::Get().root / "data/shaders/bin";

    std::error_code ec;
    std::filesystem::create_directories(shaderBin, ec);
    if (ec) {
        std::fprintf(stderr, "Failed to create shader binary directory: %s\n", shaderBin.string().c_str());
        Hockey::Log::Shutdown();
        return 1;
    }

    int compiledCount = 0;
    for (const Hockey::RequiredShaderFile& file : Hockey::RequiredShaderFiles()) {
        if (!file.compilable) {
            continue;
        }

        Hockey::ShaderDesc desc;
        desc.sourcePath = shaderSrc / file.fileName;
        desc.stage = file.stage;

        Hockey::Result<Hockey::CompiledShaderData> result = Hockey::CompileShaderFile(desc, shaderBin);
        if (!result) {
            std::fprintf(stderr, "%s\n", result.error.c_str());
            Hockey::Log::Shutdown();
            return 1;
        }

        std::printf("Compiled %s -> %s\n", desc.sourcePath.string().c_str(), result.value.outputPath.string().c_str());
        ++compiledCount;
    }

    std::printf("Compiled %d shader(s).\n", compiledCount);
    Hockey::Log::Shutdown();
    return 0;
}
