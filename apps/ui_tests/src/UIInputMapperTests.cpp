#include "Test.hpp"

#include "Hockey/Core/Event.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Mouse.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/RmlUiContext.hpp"
#include "Hockey/UI/UIInputMapper.hpp"

void RunUIInputMapperTests() {
    HockeyTest::BeginSuite("UIInputMapper");

    HK_CHECK_EQ(Hockey::UIInputMapper::MapKey(Hockey::KeyCode::Escape), Rml::Input::KI_ESCAPE);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapKey(Hockey::KeyCode::W), Rml::Input::KI_W);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapKey(Hockey::KeyCode::F12), Rml::Input::KI_F12);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapKey(Hockey::KeyCode::Unknown), Rml::Input::KI_UNKNOWN);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapMouseButton(Hockey::MouseButton::Left), 0);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapMouseButton(Hockey::MouseButton::Right), 1);
    HK_CHECK_EQ(Hockey::UIInputMapper::MapMouseButton(Hockey::MouseButton::Middle), 2);

    Hockey::Renderer renderer;
    Hockey::RmlUiContextDesc desc;
    desc.renderer = &renderer;
    desc.root = Hockey::Paths::Get().root;
    desc.name = "ui-input-test";
    desc.width = 800;
    desc.height = 450;

    Hockey::RmlUiContext context;
    HK_CHECK(context.Initialize(desc));
    HK_CHECK(context.LoadDocument("data/ui/screens/home.rml"));
    context.Update();

    Hockey::UIInputMapper mapper;
    Hockey::Event move;
    move.type = Hockey::EventType::MouseMoved;
    move.mouseX = 80.0f;
    move.mouseY = 130.0f;
    mapper.ProcessEvent(*context.RawContext(), move);

    Hockey::Event press;
    press.type = Hockey::EventType::MouseButtonPressed;
    press.mouseButton = Hockey::Mouse::ToSDLButton(Hockey::MouseButton::Left);
    mapper.ProcessEvent(*context.RawContext(), press);
    HK_CHECK(mapper.WantsMouseCapture());

    mapper.ClearCapture();
    HK_CHECK(!mapper.WantsMouseCapture());

    Hockey::Event key;
    key.type = Hockey::EventType::KeyPressed;
    key.key = Hockey::Keyboard::ToScancode(Hockey::KeyCode::Escape);
    mapper.ProcessEvent(*context.RawContext(), key);
    HK_CHECK(mapper.WantsKeyboardCapture());

    context.Shutdown();
}
