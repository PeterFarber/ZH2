#include "Test.hpp"

#include <cstring>

#include <RmlUi/Core.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/RmlUiFileInterface.hpp"
#include "Hockey/UI/RmlUiRenderInterface.hpp"
#include "Hockey/UI/RmlUiSystemInterface.hpp"

void RunRmlUiInterfaceTests() {
    HockeyTest::BeginSuite("RmlUiInterfaces");

    Hockey::Renderer renderer;
    Hockey::RmlUiRenderInterface render(renderer);

    Rml::Vertex vertices[3];
    vertices[0].position = {0.0f, 0.0f};
    vertices[0].tex_coord = {0.0f, 0.0f};
    vertices[0].colour = Rml::ColourbPremultiplied(255, 0, 0, 255);
    vertices[1].position = {8.0f, 0.0f};
    vertices[1].tex_coord = {1.0f, 0.0f};
    vertices[1].colour = Rml::ColourbPremultiplied(0, 255, 0, 255);
    vertices[2].position = {0.0f, 8.0f};
    vertices[2].tex_coord = {0.0f, 1.0f};
    vertices[2].colour = Rml::ColourbPremultiplied(0, 0, 255, 255);
    int indices[3] = {0, 1, 2};

    const Rml::CompiledGeometryHandle geometry =
        render.CompileGeometry(Rml::Span<const Rml::Vertex>(vertices, 3), Rml::Span<const int>(indices, 3));
    HK_CHECK(geometry != 0);

    const unsigned char pixels[16] = {
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
    };
    const Rml::TextureHandle texture =
        render.GenerateTexture(Rml::Span<const Rml::byte>(pixels, sizeof(pixels)), Rml::Vector2i{2, 2});
    HK_CHECK(texture != 0);

    render.EnableScissorRegion(true);
    render.SetScissorRegion(Rml::Rectanglei::FromPositionSize(Rml::Vector2i{1, 2}, Rml::Vector2i{33, 44}));
    Rml::Matrix4f transform = Rml::Matrix4f::Identity();
    render.SetTransform(&transform);
    render.RenderGeometry(geometry, Rml::Vector2f{4.0f, 5.0f}, texture);

    const auto& commands = render.DrawCommands();
    HK_CHECK_EQ(commands.size(), static_cast<std::size_t>(1));
    HK_CHECK(commands[0].geometry.IsValid());
    HK_CHECK(commands[0].texture.IsValid());
    HK_CHECK_NEAR(commands[0].translation.x, 4.0f, 0.0001);
    HK_CHECK(commands[0].scissor.has_value());
    HK_CHECK_EQ(commands[0].scissor->x, 1);
    HK_CHECK_EQ(commands[0].scissor->width, 33u);
    HK_CHECK(commands[0].transform.has_value());

    render.ReleaseGeometry(geometry);
    render.ReleaseTexture(texture);

    Hockey::RmlUiFileInterface files(Hockey::Paths::Get().root);
    Rml::FileHandle file = files.Open("data/ui/screens/home.rml");
    HK_CHECK(file != 0);
    if (file != 0) {
        char buffer[4] = {};
        HK_CHECK(files.Read(buffer, sizeof(buffer), file) > 0);
        HK_CHECK(files.Tell(file) > 0);
        files.Close(file);
    }
    HK_CHECK(files.Open("../outside.rml") == 0);
    HK_CHECK(files.Open((Hockey::Paths::Get().root / "data/ui/screens/home.rml").string()) == 0);

    Hockey::RmlUiSystemInterface system;
    Rml::String joined;
    system.JoinPath(joined, "data/ui/screens/home.rml", "../themes/hockey.rcss");
    HK_CHECK_EQ(joined, "data/ui/themes/hockey.rcss");
    HK_CHECK(system.GetElapsedTime() >= 0.0);
    HK_CHECK(system.LogMessage(Rml::Log::LT_INFO, "RmlUi interface test"));
}
