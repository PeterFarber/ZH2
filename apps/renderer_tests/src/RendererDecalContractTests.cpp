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

void RunRendererDecalContractTests() {
    HockeyTest::BeginSuite("RendererDecalContractTests");

    const std::string settings = ReadProjectFile("libs/renderer/include/Hockey/Renderer/RendererSettings.hpp");
    const std::string stats = ReadProjectFile("libs/renderer/include/Hockey/Renderer/RendererStats.hpp");
    const std::string descriptor = ReadProjectFile("libs/renderer/src/DescriptorSet.cpp");
    const std::string vulkanDescriptor = ReadProjectFile("libs/renderer/src/Vulkan/VulkanDescriptor.cpp");
    const std::string renderer = ReadProjectFile("libs/renderer/src/Renderer.cpp");
    const std::string shaderList = ReadProjectFile("libs/renderer/src/Shader.cpp");
    const std::string pbr = ReadProjectFile("data/shaders/src/pbr.frag");
    const std::string decals = ReadProjectFile("data/shaders/src/decals.glsl");
    const std::string serverCMake = ReadProjectFile("apps/dedicated_server/CMakeLists.txt");

    HK_CHECK_MSG(Contains(settings, "kRendererMaxDecals"), "renderer exposes a bounded decal budget");
    HK_CHECK_MSG(Contains(settings, "maxRenderedDecals"), "renderer settings include decal budget");
    HK_CHECK_MSG(Contains(settings, "bool decals"), "renderer settings can disable decal rendering");
    HK_CHECK_MSG(Contains(stats, "decalCount"), "renderer stats expose gathered decal count");
    HK_CHECK_MSG(Contains(descriptor, "DecalSetLayoutDesc"), "renderer defines a decal descriptor layout");
    HK_CHECK_MSG(Contains(vulkanDescriptor, "descriptorLayouts.decal"),
                 "Vulkan descriptor layouts create decal set layout");
    HK_CHECK_MSG(Contains(renderer, "view<TransformComponent, DecalComponent>"),
                 "renderer gathers DecalComponent entities");
    HK_CHECK_MSG(Contains(renderer, "UpdateDecalSet"), "renderer updates per-frame decal descriptors");
    HK_CHECK_MSG(Contains(renderer, "descriptorLayouts.global, descriptorLayouts.material, descriptorLayouts.decal"),
                 "PBR pipeline layout binds global, material, and decal sets");
    HK_CHECK_MSG(Contains(shaderList, "\"decals.glsl\""), "shader compile coverage knows the decal include");
    HK_CHECK_MSG(Contains(pbr, "#include \"decals.glsl\""), "PBR shader includes decal helpers");
    HK_CHECK_MSG(Contains(pbr, "ApplyDecals(albedo, N, vWorldPos)"), "PBR shader applies decals before lighting");
    HK_CHECK_MSG(Contains(decals, "uDecalBaseColorTex"), "decal shader samples base-color decal textures");
    HK_CHECK_MSG(Contains(decals, "uDecalNormalTex"), "decal shader samples normal decal textures");
    HK_CHECK_MSG(Contains(decals, "affectsBaseColor"), "decal shader documents base-color flag");
    HK_CHECK_MSG(Contains(decals, "affectsNormals"), "decal shader documents normal flag");
    HK_CHECK_MSG(!Contains(serverCMake, "hockey_renderer"), "dedicated server remains free of renderer linkage");
}
