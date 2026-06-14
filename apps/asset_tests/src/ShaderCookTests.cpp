#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

const char* kCommonGlsl = R"(vec3 tint() { return vec3(1.0, 0.5, 0.25); }
)";

const char* kVertexShader = R"(#version 450
layout(location = 0) in vec3 inPos;
void main() {
    gl_Position = vec4(inPos, 1.0);
}
)";

const char* kFragmentShader = R"(#version 450
#include "common.glsl"
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(tint(), 1.0);
}
)";

const char* kBrokenShader = R"(#version 450
layout(location = 0) out vec4 outColor;
void main() {
    outColor = this_is_not_valid_glsl
}
)";

} // namespace

void RunShaderCookTests() {
    HockeyTest::BeginSuite("ShaderCookTests");

    const fs::path workspace = Paths::TempFile("shader_ws");
    FileSystem::Remove(workspace);
    const fs::path shaderDir = workspace / "data" / "raw" / "shaders";

    FileSystem::WriteTextFile(shaderDir / "common.glsl", kCommonGlsl);
    FileSystem::WriteTextFile(shaderDir / "mesh.vert", kVertexShader);
    FileSystem::WriteTextFile(shaderDir / "pbr.frag", kFragmentShader);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");

    // Discovery + classification.
    AssetMetadata* vertMeta = manager.Database().FindByRawPath("data/raw/shaders/mesh.vert");
    AssetMetadata* fragMeta = manager.Database().FindByRawPath("data/raw/shaders/pbr.frag");
    AssetMetadata* commonMeta = manager.Database().FindByRawPath("data/raw/shaders/common.glsl");
    HK_CHECK_MSG(vertMeta != nullptr && vertMeta->type == AssetType::Shader, "vertex is shader");
    HK_CHECK_MSG(fragMeta != nullptr && fragMeta->type == AssetType::Shader, "fragment is shader");
    HK_CHECK_MSG(commonMeta != nullptr && commonMeta->type == AssetType::Shader, "include lib is shader");

    // Include dependency tracking: pbr.frag -> common.glsl.
    bool fragDependsOnCommon = false;
    if (fragMeta != nullptr && commonMeta != nullptr) {
        for (const AssetID dep : fragMeta->dependencies) {
            fragDependsOnCommon = fragDependsOnCommon || dep == commonMeta->id;
        }
    }
    HK_CHECK_MSG(fragDependsOnCommon, "fragment tracks include dependency");

    // Compile to SPIR-V.
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");
    vertMeta = manager.Database().FindByRawPath("data/raw/shaders/mesh.vert");
    HK_CHECK_MSG(vertMeta != nullptr && vertMeta->cooked, "vertex cooked");
    HK_CHECK_MSG(vertMeta != nullptr && FileSystem::Exists(workspace / vertMeta->cookedPath), "spv written");

    // Load cooked SPIR-V.
    const AssetID vertId = vertMeta->id;
    Result<std::shared_ptr<ShaderAsset>> vertShader = manager.Load<ShaderAsset>(vertId);
    HK_CHECK_MSG(static_cast<bool>(vertShader), "load cooked vertex shader");
    if (vertShader) {
        HK_CHECK_MSG(!vertShader.value->spirv.empty(), "spirv non-empty");
        HK_CHECK_MSG(vertShader.value->spirv[0] == 0x07230203u, "spirv magic number");
        HK_CHECK_MSG(vertShader.value->stage == ShaderAssetStage::Vertex, "vertex stage");
    }

    Result<std::shared_ptr<ShaderAsset>> fragShader = manager.Load<ShaderAsset>(fragMeta->id);
    HK_CHECK_MSG(static_cast<bool>(fragShader), "load cooked fragment shader (with include)");
    if (fragShader) {
        HK_CHECK_MSG(fragShader.value->stage == ShaderAssetStage::Fragment, "fragment stage");
    }

    // Compile error is detected and reported.
    FileSystem::WriteTextFile(shaderDir / "broken.frag", kBrokenShader);
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "reimport with broken shader");
    AssetMetadata* brokenMeta = manager.Database().FindByRawPath("data/raw/shaders/broken.frag");
    HK_CHECK_MSG(brokenMeta != nullptr, "broken shader discovered");
    const Status cookBroken = manager.CookAsset(brokenMeta->id);
    HK_CHECK_MSG(!cookBroken, "broken shader fails to cook");
    HK_CHECK_MSG(cookBroken.error.find("compile failed") != std::string::npos, "compile error reported");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
