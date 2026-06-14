#include "Hockey/Renderer/Shader.hpp"

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

#include "Hockey/Core/Log.hpp"

namespace Hockey {

namespace {

shaderc_shader_kind ToShadercKind(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::Vertex:
        return shaderc_glsl_vertex_shader;
    case ShaderStage::Fragment:
        return shaderc_glsl_fragment_shader;
    case ShaderStage::Compute:
        return shaderc_glsl_compute_shader;
    }
    return shaderc_glsl_vertex_shader;
}

std::string ReadFile(const std::filesystem::path& path, bool& ok) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        ok = false;
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    ok = true;
    return ss.str();
}

// Resolves #include "file" against the including file's directory.
class FileIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    explicit FileIncluder(std::filesystem::path sourceDir) : m_SourceDir(std::move(sourceDir)) {}

    shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type /*type*/,
                                       const char* requestingSource, size_t /*includeDepth*/) override {
        std::filesystem::path base = std::filesystem::path(requestingSource).parent_path();
        if (base.empty()) {
            base = m_SourceDir;
        }
        std::filesystem::path full = base / requestedSource;
        bool ok = false;
        std::string content = ReadFile(full, ok);
        if (!ok) {
            full = m_SourceDir / requestedSource;
            content = ReadFile(full, ok);
        }

        auto* data = new IncludeData();
        if (ok) {
            data->name = full.generic_string();
            data->content = std::move(content);
        }
        auto* result = new shaderc_include_result();
        result->source_name = data->name.c_str();
        result->source_name_length = data->name.size();
        result->content = data->content.c_str();
        result->content_length = data->content.size();
        result->user_data = data;
        return result;
    }

    void ReleaseInclude(shaderc_include_result* result) override {
        delete static_cast<IncludeData*>(result->user_data);
        delete result;
    }

private:
    struct IncludeData {
        std::string name;
        std::string content;
    };
    std::filesystem::path m_SourceDir;
};

} // namespace

ShaderStage ShaderStageFromExtension(const std::filesystem::path& path) {
    const std::string ext = path.extension().string();
    if (ext == ".frag") {
        return ShaderStage::Fragment;
    }
    if (ext == ".comp") {
        return ShaderStage::Compute;
    }
    return ShaderStage::Vertex;
}

Result<CompiledShaderData> CompileShaderFile(const ShaderDesc& desc, const std::filesystem::path& binaryDir) {
    bool ok = false;
    const std::string source = ReadFile(desc.sourcePath, ok);
    if (!ok) {
        return Result<CompiledShaderData>::Fail("Shader source not found: " + desc.sourcePath.string());
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetIncluder(std::make_unique<FileIncluder>(desc.sourcePath.parent_path()));
    for (const std::string& define : desc.defines) {
        const size_t eq = define.find('=');
        if (eq == std::string::npos) {
            options.AddMacroDefinition(define);
        } else {
            options.AddMacroDefinition(define.substr(0, eq), define.substr(eq + 1));
        }
    }

    const std::string name = desc.sourcePath.filename().generic_string();
    const shaderc::SpvCompilationResult result =
        compiler.CompileGlslToSpv(source, ToShadercKind(desc.stage), name.c_str(), desc.entryPoint.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        HK_CORE_ERROR("Shader compile failed ({}): {}", name, result.GetErrorMessage());
        return Result<CompiledShaderData>::Fail("Shader compile failed (" + name + "): " + result.GetErrorMessage());
    }

    CompiledShaderData data;
    data.spirv.assign(result.cbegin(), result.cend());

    if (!binaryDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(binaryDir, ec);
        std::filesystem::path outPath = binaryDir / (name + ".spv");
        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            return Result<CompiledShaderData>::Fail("Failed to write SPIR-V: " + outPath.string());
        }
        out.write(reinterpret_cast<const char*>(data.spirv.data()),
                  static_cast<std::streamsize>(data.spirv.size() * sizeof(uint32_t)));
        data.outputPath = outPath;
    }

    return Result<CompiledShaderData>::Ok(std::move(data));
}

const std::vector<RequiredShaderFile>& RequiredShaderFiles() {
    static const std::vector<RequiredShaderFile> kFiles = {
        {"common.glsl", false, ShaderStage::Vertex},
        {"mesh.vert", true, ShaderStage::Vertex},
        {"pbr.frag", true, ShaderStage::Fragment},
        {"depth_only.vert", true, ShaderStage::Vertex},
        {"shadow.vert", true, ShaderStage::Vertex},
        {"shadow.frag", true, ShaderStage::Fragment},
        {"fullscreen_triangle.vert", true, ShaderStage::Vertex},
        {"tonemap.frag", true, ShaderStage::Fragment},
        {"bloom_downsample.frag", true, ShaderStage::Fragment},
        {"bloom_upsample.frag", true, ShaderStage::Fragment},
        {"ssao.frag", true, ShaderStage::Fragment},
        {"ssao_blur.frag", true, ShaderStage::Fragment},
        {"fxaa.frag", true, ShaderStage::Fragment},
        {"debug_line.vert", true, ShaderStage::Vertex},
        {"debug_line.frag", true, ShaderStage::Fragment},
        {"debug_shape.vert", true, ShaderStage::Vertex},
        {"debug_shape.frag", true, ShaderStage::Fragment},
    };
    return kFiles;
}

} // namespace Hockey
