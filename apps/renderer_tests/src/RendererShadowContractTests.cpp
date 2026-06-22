#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

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

void RunRendererShadowContractTests() {
    HockeyTest::BeginSuite("RendererShadowContractTests");

    const std::string renderer = ReadProjectFile("libs/renderer/src/Renderer.cpp");
    const std::string vulkanBuffer = ReadProjectFile("libs/renderer/src/Vulkan/VulkanBuffer.cpp");
    const std::string commonGlsl = ReadProjectFile("data/shaders/src/common.glsl");
    const std::string meshVert = ReadProjectFile("data/shaders/src/mesh.vert");
    const std::string pbrFrag = ReadProjectFile("data/shaders/src/pbr.frag");
    const std::string shadowFrag = ReadProjectFile("data/shaders/src/shadow.frag");

    HK_CHECK_MSG(Contains(vulkanBuffer, "vmaFlushAllocation"), "mapped buffer writes are flushed for GPU visibility");
    HK_CHECK_MSG(Contains(renderer, "item.receivesShadows = mr.receivesShadows"),
                 "MeshRendererComponent::receivesShadows reaches draw items");
    HK_CHECK_MSG(Contains(renderer, "settings.contactShadows ? 1.0f : 0.0f"),
                 "RendererSettings::contactShadows reaches draw push constants");
    HK_CHECK_MSG(Contains(meshVert, "vec4 flags"), "mesh push constants carry per-draw flags");
    HK_CHECK_MSG(Contains(pbrFrag, "uPush.flags.x > 0.5"), "PBR shader honors receivesShadows");
    HK_CHECK_MSG(Contains(pbrFrag, "uPush.flags.y > 0.5"), "PBR shader honors contactShadows");
    HK_CHECK_MSG(Contains(shadowFrag, "discard"), "shadow fragment shader alpha-tests masked materials");
    HK_CHECK_MSG(Contains(renderer, "vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout"),
                 "shadow pass binds material descriptors for alpha-tested casters");
    HK_CHECK_MSG(Contains(renderer, "cascadeTexelSizes"), "directional cascades upload per-cascade world texel sizes");
    HK_CHECK_MSG(Contains(renderer, "constexpr float lambda = 0.95f"),
                 "directional cascades keep high near-cascade texel density with long camera far clips");
    HK_CHECK_MSG(!Contains(renderer, "constexpr float lambda = 0.85f"),
                 "directional cascade split blend is not too uniform for close-up shadow quality");
    HK_CHECK_MSG(Contains(commonGlsl, "uScene.cascadeTexelSizes"),
                 "directional shadow receiver offset is scaled by cascade texel size");
    HK_CHECK_MSG(!Contains(commonGlsl, "normalize(normal) * 0.035"),
                 "directional shadow receiver offset is not a fixed world-space distance");
    HK_CHECK_MSG(Contains(commonGlsl, "nextValid"), "directional cascade blending ignores invalid neighboring cascades");
    HK_CHECK_MSG(Contains(meshVert, "transpose(inverse(mat3(uPush.model)))"),
                 "mesh normals use inverse-transpose model transform for non-uniform scale");
    HK_CHECK_MSG(!Contains(meshVert, "normalMat = mat3(uPush.model)"),
                 "mesh normals are not transformed directly by the model matrix");
}
