#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/RmlUiContext.hpp"
#include "Hockey/UI/RmlUiRenderInterface.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

void RunRmlUiContextTests() {
    HockeyTest::BeginSuite("RmlUiContext");

    Hockey::Renderer renderer;
    Hockey::RmlUiContextDesc desc;
    desc.renderer = &renderer;
    desc.root = Hockey::Paths::Get().root;
    desc.name = "ui-context-test";
    desc.width = 800;
    desc.height = 450;

    Hockey::RmlUiContext context;
    HK_CHECK(context.Initialize(desc));
    HK_CHECK(context.IsInitialized());
    HK_CHECK(context.RawContext() != nullptr);
    HK_CHECK(context.LoadDocument("data/ui/screens/home.rml"));
    HK_CHECK_EQ(context.LoadedDocumentCount(), 1);
    bool clicked = false;
    HK_CHECK(context.BindClickAction("play-offline", [&clicked]() { clicked = true; }));
    context.RawContext()->GetDocument(0)->GetElementById("play-offline")->Click();
    HK_CHECK(clicked);

    context.SetDimensions(1024, 576);
    context.Update();
    HK_CHECK(context.RenderInterface() != nullptr);

    context.Shutdown();
    HK_CHECK(!context.IsInitialized());
}
