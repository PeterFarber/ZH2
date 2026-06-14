#include "Hockey/Assets/Cookers/ShaderCooker.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Assets/Importers/ShaderImporter.hpp"
#include "Hockey/Assets/Runtime/ShaderLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

shaderc_shader_kind ToShadercKind(ShaderAssetStage stage) {
    switch (stage) {
    case ShaderAssetStage::Fragment:
        return shaderc_glsl_fragment_shader;
    case ShaderAssetStage::Compute:
        return shaderc_glsl_compute_shader;
    case ShaderAssetStage::Vertex:
    default:
        return shaderc_glsl_vertex_shader;
    }
}

std::string ReadFile(const fs::path& path, bool& ok) {
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

// Resolves #include "file" against the including file's directory, falling back
// to the shader root.
class FileIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    explicit FileIncluder(fs::path sourceDir) : m_SourceDir(std::move(sourceDir)) {}

    shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type, const char* requestingSource,
                                       size_t) override {
        fs::path base = fs::path(requestingSource).parent_path();
        if (base.empty()) {
            base = m_SourceDir;
        }
        fs::path full = base / requestedSource;
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
    fs::path m_SourceDir;
};

} // namespace

CookResult ShaderCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    const fs::path rawAbsolute = context.projectRoot / metadata.rawPath;
    if (context.database != nullptr) {
        result.dependencies =
            ShaderImporter::ScanIncludeDependencies(rawAbsolute, context.projectRoot, *context.database);
    }

    const ShaderAssetStage stage = ShaderStageFromExtension(metadata.rawPath.extension().string());

    const fs::path shaderDir = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(AssetType::Shader);

    // .glsl include libraries are not standalone-compilable; copy them verbatim
    // so they remain tracked and available next to the compiled shaders.
    if (stage == ShaderAssetStage::Unknown) {
        const Result<std::string> source = FileSystem::ReadTextFile(rawAbsolute);
        if (!source) {
            result.success = false;
            result.error = source.error;
            return result;
        }
        const fs::path cookedAbsolute = shaderDir / (metadata.id.ToString() + ".glsl");
        const Status wrote = FileSystem::WriteTextFile(cookedAbsolute, source.value);
        if (!wrote) {
            result.success = false;
            result.error = wrote.error;
            return result;
        }
        result.success = true;
        result.cookedPath = cookedAbsolute;
        return result;
    }

    bool ok = false;
    const std::string source = ReadFile(rawAbsolute, ok);
    if (!ok) {
        result.success = false;
        result.error = "shader source not found: " + rawAbsolute.string();
        return result;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetIncluder(std::make_unique<FileIncluder>(rawAbsolute.parent_path()));

    const std::string name = metadata.rawPath.filename().generic_string();
    const shaderc::SpvCompilationResult compiled =
        compiler.CompileGlslToSpv(source, ToShadercKind(stage), name.c_str(), "main", options);

    if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
        result.success = false;
        result.error = "shader compile failed (" + name + "): " + compiled.GetErrorMessage();
        return result;
    }

    const std::vector<uint32_t> spirv(compiled.cbegin(), compiled.cend());
    const fs::path cookedAbsolute =
        shaderDir / (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Shader));
    const Status wrote = ShaderLoader::WriteSpv(cookedAbsolute, spirv);
    if (!wrote) {
        result.success = false;
        result.error = wrote.error;
        return result;
    }

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
