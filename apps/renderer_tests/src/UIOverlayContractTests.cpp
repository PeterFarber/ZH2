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
}
