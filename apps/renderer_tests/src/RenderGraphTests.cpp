#include "Test.hpp"

#include "Hockey/Renderer/RenderGraph.hpp"

using namespace Hockey;

namespace {

TextureDesc ColorDesc(const char* name) {
    TextureDesc desc;
    desc.format = TextureFormat::RGBA16F;
    desc.usageFlags = TextureUsage_RenderTarget | TextureUsage_Sampled;
    desc.debugName = name;
    return desc;
}

TextureDesc DepthDesc(const char* name) {
    TextureDesc desc;
    desc.format = TextureFormat::Depth32F;
    desc.usageFlags = TextureUsage_DepthStencil | TextureUsage_Sampled;
    desc.debugName = name;
    return desc;
}

// A pass that writes a single color attachment, optionally reading inputs.
RenderPassDesc MakePass(const char* name, TextureHandle write, std::vector<TextureHandle> reads = {}) {
    RenderPassDesc pass;
    pass.name = name;
    pass.reads = std::move(reads);
    if (write.IsValid()) {
        ColorAttachment color;
        color.texture = write;
        pass.colorAttachments.push_back(color);
    }
    return pass;
}

} // namespace

void RunRenderGraphTests() {
    HockeyTest::BeginSuite("RenderGraphTests");

    // ---- can add pass / can add resource ----
    {
        RenderGraph graph;
        graph.Resize(1280, 720);
        const TextureHandle color = graph.CreateTransientTexture(ColorDesc("Color"));
        const TextureHandle depth = graph.CreateTransientTexture(DepthDesc("Depth"));
        HK_CHECK(color.IsValid());
        HK_CHECK(depth.IsValid());
        HK_CHECK(color != depth);
        HK_CHECK_EQ(graph.ResourceCount(), static_cast<size_t>(2));

        RenderPassDesc pass = MakePass("MainOpaque", color);
        pass.depth.texture = depth;
        const RenderPassHandle handle = graph.AddPass(pass);
        HK_CHECK(handle.IsValid());
        HK_CHECK_EQ(graph.PassCount(), static_cast<size_t>(1));

        // Render-area-relative transient picks up the render area dimensions.
        const RenderGraph::Resource* colorRes = graph.GetResource(color);
        HK_CHECK(colorRes != nullptr);
        HK_CHECK_EQ(colorRes->desc.width, 1280u);
        HK_CHECK_EQ(colorRes->desc.height, 720u);
    }

    // ---- compile orders passes (producer before consumer, even out of order) ----
    {
        RenderGraph graph;
        graph.Resize(800, 600);
        const TextureHandle gbuffer = graph.CreateTransientTexture(ColorDesc("GBuffer"));
        const TextureHandle lit = graph.CreateTransientTexture(ColorDesc("Lit"));

        // Add the consumer first, then the producer. Compile must still order
        // the producer ("Geometry") before the consumer ("Lighting").
        graph.AddPass(MakePass("Lighting", lit, {gbuffer}));
        graph.AddPass(MakePass("Geometry", gbuffer));

        const Status status = graph.Compile();
        HK_CHECK(static_cast<bool>(status));
        HK_CHECK(graph.IsCompiled());
        HK_CHECK_EQ(graph.ExecutionOrder().size(), static_cast<size_t>(2));
        HK_CHECK(graph.PassNameAtOrder(0) == "Geometry");
        HK_CHECK(graph.PassNameAtOrder(1) == "Lighting");

        // Lifetimes: GBuffer first used by Geometry (order 0), last by Lighting (1).
        const RenderGraph::Resource* gbufferRes = graph.GetResource(gbuffer);
        HK_CHECK(gbufferRes != nullptr);
        HK_CHECK_EQ(gbufferRes->firstUse, 0);
        HK_CHECK_EQ(gbufferRes->lastUse, 1);
    }

    // ---- resize invalidates / recreates resources ----
    {
        RenderGraph graph;
        graph.Resize(1280, 720);
        const TextureHandle full = graph.CreateTransientTexture(ColorDesc("Full"), RenderGraphSizing::RenderArea(1.0f));
        const TextureHandle half = graph.CreateTransientTexture(ColorDesc("Half"), RenderGraphSizing::RenderArea(0.5f));
        const TextureHandle fixed = graph.CreateTransientTexture(
            [] {
                TextureDesc d = ColorDesc("Shadow");
                d.width = 2048;
                d.height = 2048;
                return d;
            }(),
            RenderGraphSizing::Absolute());
        graph.AddPass(MakePass("MakeHalf", half));
        graph.AddPass(MakePass("MakeShadow", fixed));
        graph.AddPass(MakePass("Use", full, {half, fixed}));
        HK_CHECK(static_cast<bool>(graph.Compile()));

        // Simulate the backend having allocated everything.
        graph.MarkResourcesRealized();
        HK_CHECK(!graph.GetResource(full)->dirty);

        graph.Resize(1920, 1080);
        const RenderGraph::Resource* fullRes = graph.GetResource(full);
        const RenderGraph::Resource* halfRes = graph.GetResource(half);
        const RenderGraph::Resource* fixedRes = graph.GetResource(fixed);
        HK_CHECK_EQ(fullRes->desc.width, 1920u);
        HK_CHECK_EQ(fullRes->desc.height, 1080u);
        HK_CHECK(fullRes->dirty); // resize invalidated the relative resource
        HK_CHECK_EQ(halfRes->desc.width, 960u);
        HK_CHECK_EQ(halfRes->desc.height, 540u);
        HK_CHECK(halfRes->dirty);
        // Absolute-sized resource is untouched by resize.
        HK_CHECK_EQ(fixedRes->desc.width, 2048u);
        HK_CHECK(!fixedRes->dirty);
    }

    // ---- missing dependency reports error ----
    {
        RenderGraph graph;
        graph.Resize(640, 480);
        const TextureHandle output = graph.CreateTransientTexture(ColorDesc("Output"));
        const TextureHandle phantom = graph.CreateTransientTexture(ColorDesc("Phantom"));
        // "Output" is read but nobody writes "Phantom" -> missing dependency.
        graph.AddPass(MakePass("Final", output, {phantom}));

        const Status status = graph.Compile();
        HK_CHECK(!static_cast<bool>(status));
        HK_CHECK(!graph.IsCompiled());
        HK_CHECK(!graph.LastError().empty());
    }

    // ---- imported resource satisfies reads without a producer ----
    {
        RenderGraph graph;
        graph.Resize(640, 480);
        const TextureHandle backbuffer = graph.ImportTexture(ColorDesc("Swapchain"));
        const TextureHandle scene = graph.CreateTransientTexture(ColorDesc("Scene"));
        graph.AddPass(MakePass("Scene", scene));
        // Present reads the scene and writes the imported backbuffer.
        graph.AddPass(MakePass("Present", backbuffer, {scene}));

        const Status status = graph.Compile();
        HK_CHECK(static_cast<bool>(status));
        HK_CHECK(graph.PassNameAtOrder(0) == "Scene");
        HK_CHECK(graph.PassNameAtOrder(1) == "Present");
        // Imported resources are not owned/allocated by the graph.
        HK_CHECK(graph.GetResource(backbuffer)->imported);
        HK_CHECK(!graph.GetResource(backbuffer)->dirty);
    }

    // ---- cycle detection ----
    {
        RenderGraph graph;
        graph.Resize(640, 480);
        const TextureHandle a = graph.CreateTransientTexture(ColorDesc("A"));
        const TextureHandle b = graph.CreateTransientTexture(ColorDesc("B"));
        // PassX writes A, reads B; PassY writes B, reads A -> cyclic dependency.
        graph.AddPass(MakePass("PassX", a, {b}));
        graph.AddPass(MakePass("PassY", b, {a}));

        const Status status = graph.Compile();
        HK_CHECK(!static_cast<bool>(status));
        HK_CHECK(!graph.LastError().empty());
    }

    // ---- clear resets the graph ----
    {
        RenderGraph graph;
        graph.Resize(100, 100);
        const TextureHandle t = graph.CreateTransientTexture(ColorDesc("T"));
        graph.AddPass(MakePass("P", t));
        HK_CHECK(static_cast<bool>(graph.Compile()));
        graph.Clear();
        HK_CHECK_EQ(graph.PassCount(), static_cast<size_t>(0));
        HK_CHECK_EQ(graph.ResourceCount(), static_cast<size_t>(0));
        HK_CHECK(!graph.IsCompiled());
    }
}
