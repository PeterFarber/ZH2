#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/Renderer/UIOverlay.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunUIOverlayContractTests() {
    HockeyTest::BeginSuite("UIOverlayContracts");

    Hockey::UIOverlayGeometryHandle geometry;
    Hockey::UIOverlayTextureHandle texture;
    HK_CHECK(!geometry.IsValid());
    HK_CHECK(!texture.IsValid());
    geometry.id = 7;
    texture.id = 11;
    HK_CHECK(geometry.IsValid());
    HK_CHECK(texture.IsValid());

    Hockey::UIOverlayVertex vertex;
    vertex.position = {12.0f, 34.0f};
    vertex.texCoord = {0.25f, 0.75f};
    vertex.color = {1.0f, 0.5f, 0.25f, 0.5f};
    HK_CHECK_NEAR(vertex.position.x, 12.0f, 0.0001);
    HK_CHECK_NEAR(vertex.texCoord.y, 0.75f, 0.0001);

    Hockey::UIOverlayDrawCommand command;
    command.geometry = geometry;
    command.texture = texture;
    command.translation = {4.0f, 5.0f};
    command.scissor = Hockey::UIOverlayScissor{1, 2, 128, 64};
    HK_CHECK(command.scissor.has_value());
    HK_CHECK_EQ(command.scissor->width, 128u);

    const std::string rendererHeader = ReadProjectFile("libs/renderer/include/Hockey/Renderer/Renderer.hpp");
    HK_CHECK_MSG(Contains(rendererHeader, "CreateUIOverlayGeometry"), "Renderer exposes UI geometry creation");
    HK_CHECK_MSG(Contains(rendererHeader, "ReleaseUIOverlayGeometry"), "Renderer exposes UI geometry release");
    HK_CHECK_MSG(Contains(rendererHeader, "CreateUIOverlayTexture"), "Renderer exposes UI texture creation");
    HK_CHECK_MSG(Contains(rendererHeader, "ReleaseUIOverlayTexture"), "Renderer exposes UI texture release");
    HK_CHECK_MSG(Contains(rendererHeader, "RenderUIOverlay"), "Renderer exposes ordered UI overlay rendering");
    HK_CHECK_MSG(Contains(rendererHeader, "RenderUIOverlayToTarget"),
                 "Renderer exposes offscreen UI overlay rendering for editor Client Preview");

    const std::string rendererSource = ReadProjectFile("libs/renderer/src/Renderer.cpp");
    HK_CHECK_MSG(Contains(rendererSource, "BuildUIOverlayPipeline"), "Renderer builds a UI overlay pipeline");
    HK_CHECK_MSG(Contains(rendererSource, "RecordUIOverlay"), "Renderer records UI overlay draw commands");
    HK_CHECK_MSG(Contains(rendererSource, "vkCmdDrawIndexed(cmd, geometry.indexCount"),
                 "Renderer draws UI overlay geometry on the GPU");
    HK_CHECK_MSG(Contains(rendererSource, "VK_ATTACHMENT_LOAD_OP_LOAD"), "UI overlay loads existing scene output");
    HK_CHECK_MSG(Contains(rendererSource, "ClampUIOverlayScissor"), "UI overlay clamps top-left scissor rectangles");
    HK_CHECK_MSG(Contains(rendererSource, "uiOverlayCommands.clear()"), "UI overlay commands clear after each frame");
    HK_CHECK_MSG(Contains(rendererSource, "RenderUIOverlayToTarget"), "Renderer implements offscreen UI overlay path");
    HK_CHECK_MSG(Contains(rendererSource, "ResolveRenderTarget(target)"),
                 "Offscreen UI overlay validates renderer-owned target handles");

    const std::string pipelineSource = ReadProjectFile("libs/renderer/src/Vulkan/VulkanPipeline.cpp");
    HK_CHECK_MSG(Contains(pipelineSource, "BlendMode::PremultipliedAlpha"),
                 "Renderer has premultiplied-alpha blend state for UI");
    HK_CHECK_MSG(Contains(pipelineSource, "VK_BLEND_FACTOR_ONE"),
                 "Premultiplied-alpha UI blend uses one as the source factor");

    const std::string shaderList = ReadProjectFile("libs/renderer/src/Shader.cpp");
    HK_CHECK_MSG(Contains(shaderList, "\"ui.vert\""), "UI vertex shader is required");
    HK_CHECK_MSG(Contains(shaderList, "\"ui.frag\""), "UI fragment shader is required");
    const std::string uiFrag = ReadProjectFile("data/shaders/src/ui.frag");
    HK_CHECK_MSG(Contains(uiFrag, "texture(uTexture"), "UI fragment shader samples renderer-owned UI textures");
}
