#include "Test.hpp"

#include <cstdint>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/EventQueue.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/Mesh.hpp"
#include "Hockey/Renderer/Renderer.hpp"

using namespace Hockey;

namespace {

void PumpEvents(Window& window) {
    EventQueue queue;
    window.PollEvents(queue);
    Event e;
    while (queue.Poll(e)) {
        // drain
    }
}

uint64_t MeshId(AssetManager& assets, const char* rawPath) {
    const AssetMetadata* meta = assets.Database().FindByRawPath(rawPath);
    return meta != nullptr ? meta->id.Value() : 0;
}

uint64_t MaterialId(AssetManager& assets, const char* rawPath) {
    const AssetMetadata* meta = assets.Database().FindByRawPath(rawPath);
    return meta != nullptr ? meta->id.Value() : 0;
}

MeshDesc MakeSmokeTriangleMesh() {
    MeshDesc desc;
    desc.debugName = "SmokeTestTriangle";
    desc.vertices = {
        Vertex{{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        Vertex{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
    };
    desc.indices = {0, 1, 2};
    return desc;
}

// Builds a small scene: ground plane, an opaque cube + sphere, a transparent
// glass sphere, plus a directional light and ambient environment.
void PopulateScene(Scene& scene, AssetManager& assets) {
    Entity env = scene.CreateEntity("Environment");
    auto& environment = env.AddComponent<EnvironmentComponent>();
    environment.ambientColor = {0.05f, 0.06f, 0.08f};
    environment.ambientIntensity = 1.0f;

    Entity sun = scene.CreateEntity("Sun");
    sun.GetComponent<TransformComponent>().localRotation = {-50.0f, -30.0f, 0.0f};
    auto& light = sun.AddComponent<LightComponent>();
    light.type = LightComponent::Type::Directional;
    light.color = {1.0f, 0.97f, 0.9f};
    light.intensity = 3.0f;

    Entity floor = scene.CreateEntity("Floor");
    auto& floorMesh = floor.AddComponent<MeshRendererComponent>();
    floorMesh.meshAsset = MeshId(assets, "data/raw/meshes/plane/plane_mesh.mesh.yaml");
    floorMesh.materialAsset = MaterialId(assets, "data/raw/materials/Ice.material.yaml");
    floor.GetComponent<TransformComponent>().localScale = {60.96f, 1.0f, 25.91f};

    Entity cube = scene.CreateEntity("Cube");
    cube.GetComponent<TransformComponent>().localPosition = {-1.5f, 0.5f, 0.0f};
    auto& cubeMesh = cube.AddComponent<MeshRendererComponent>();
    cubeMesh.meshAsset = MeshId(assets, "data/raw/meshes/cube/cube_mesh.mesh.yaml");
    cubeMesh.materialAsset = MaterialId(assets, "data/raw/materials/HomeJersey.material.yaml");

    Entity sphere = scene.CreateEntity("Sphere");
    sphere.GetComponent<TransformComponent>().localPosition = {1.5f, 0.5f, 0.0f};
    auto& sphereMesh = sphere.AddComponent<MeshRendererComponent>();
    sphereMesh.meshAsset = MeshId(assets, "data/raw/meshes/sphere/sphere_mesh.mesh.yaml");
    sphereMesh.materialAsset = MaterialId(assets, "data/raw/materials/AwayJersey.material.yaml");

    Entity glass = scene.CreateEntity("Glass");
    glass.GetComponent<TransformComponent>().localPosition = {0.0f, 0.5f, 1.5f};
    auto& glassMesh = glass.AddComponent<MeshRendererComponent>();
    glassMesh.meshAsset = MeshId(assets, "data/raw/meshes/sphere/sphere_mesh.mesh.yaml");
    glassMesh.materialAsset = MaterialId(assets, "data/raw/materials/Glass.material.yaml");
}

CameraRenderData MakeOrbitCamera(float aspect) {
    TransformComponent transform;
    transform.localPosition = {0.0f, 3.0f, 6.0f};
    transform.localRotation = {-25.0f, 0.0f, 0.0f};
    CameraComponent camera;
    camera.fovDegrees = 60.0f;
    return BuildCameraRenderData(transform, camera, aspect);
}

} // namespace

// Creates a real window + Vulkan device, renders frames, resizes, and shuts
// down. Skips gracefully (no recorded failures) when no display/GPU is present.
void RunRendererSmokeTests() {
    HockeyTest::BeginSuite("RendererSmokeTests");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        HK_CORE_WARN("RendererSmokeTests skipped: SDL video init failed ({})", SDL_GetError());
        return;
    }

    Window window;
    WindowDesc desc;
    desc.title = "Hockey Renderer Smoke Test";
    desc.width = 640;
    desc.height = 360;
    desc.vulkan = true;
    if (!window.Create(desc)) {
        HK_CORE_WARN("RendererSmokeTests skipped: window creation failed (driver={}, err={})",
                     SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none", SDL_GetError());
        SDL_Quit();
        return;
    }

    Renderer renderer;
    RendererInitInfo info;
    info.window = &window;
    info.settings = MakeDefaultRendererSettings();
    info.enableValidation = true;
    info.enableDebugMarkers = true;
    info.shaderSourceDirectory = Paths::Get().root / "data/shaders/src";
    info.shaderBinaryDirectory = Paths::Get().root / "data/shaders/bin";

    const Status initStatus = renderer.Init(info);
    if (!initStatus) {
        HK_CORE_WARN("RendererSmokeTests skipped: renderer init failed ({})", initStatus.error);
        window.Destroy();
        SDL_Quit();
        return;
    }

    HK_CHECK(renderer.IsInitialized());
    HK_CHECK(renderer.CanRender());

    AssetManager assetManager;
    const Status assetStatus = assetManager.Init(AssetManager::DefaultCreateInfo(Paths::Get().root));
    HK_CHECK_MSG(static_cast<bool>(assetStatus), "asset manager initializes for renderer smoke test");
    renderer.SetAssetManager(&assetManager);

    // Create a small texture from CPU pixel data and validate the handle.
    const uint32_t kPixels[4] = {0xFFFFFFFFu, 0xFF808080u, 0xFF404040u, 0xFFFFFFFFu};
    TextureDesc texDesc;
    texDesc.width = 2;
    texDesc.height = 2;
    texDesc.format = TextureFormat::RGBA8;
    texDesc.debugName = "SmokeTestTexture";
    texDesc.initialData = kPixels;
    texDesc.initialDataSize = sizeof(kPixels);
    const TextureHandle tex = renderer.CreateTexture(texDesc);
    HK_CHECK(tex.IsValid());

    // Custom mesh + material creation returns valid handles.
    const MeshHandle customMesh = renderer.CreateMesh(MakeSmokeTriangleMesh());
    HK_CHECK(customMesh.IsValid());
    MaterialDesc matDesc;
    matDesc.baseColor = {0.2f, 0.6f, 0.9f, 1.0f};
    matDesc.debugName = "SmokeTestMaterial";
    const MaterialHandle customMaterial = renderer.CreateMaterial(matDesc);
    HK_CHECK(customMaterial.IsValid());

    // Render several frames at the initial size.
    for (int i = 0; i < 8; ++i) {
        PumpEvents(window);
        renderer.BeginFrame();
        renderer.EndFrame();
    }

    // Resize and render more frames.
    SDL_SetWindowSize(window.SDLHandle(), 800, 600);
    for (int i = 0; i < 4; ++i) {
        PumpEvents(window);
    }
    renderer.Resize(800, 600);
    for (int i = 0; i < 8; ++i) {
        PumpEvents(window);
        renderer.BeginFrame();
        renderer.EndFrame();
    }

    // Render an actual scene through the gather + forward PBR path.
    {
        Scene scene("SmokeScene");
        PopulateScene(scene, assetManager);
        const float aspect = 800.0f / 600.0f;
        const CameraRenderData camera = MakeOrbitCamera(aspect);
        for (int i = 0; i < 8; ++i) {
            PumpEvents(window);
            renderer.BeginFrame();
            renderer.RenderScene(scene, camera);
            renderer.EndFrame();
        }
        // The frame after a scene render should have issued draws for floor,
        // cube, sphere, and transparent glass.
        HK_CHECK(renderer.GetStats().drawCalls >= 4);
        HK_CHECK(renderer.GetStats().triangleCount > 0);
        // Default settings (High shadows) render 4 cascades of opaque geometry,
        // plus the HDR -> bloom -> tonemap -> FXAA post chain.
        HK_CHECK(renderer.GetStats().shadowDrawCalls > 0);
        HK_CHECK(renderer.GetStats().postProcessPasses > 0);
    }

    // Exercise the no-shadow + no-bloom + no-AA settings path.
    {
        RendererSettings noPost = renderer.GetSettings();
        noPost.shadowQuality = ShadowQuality::Off;
        noPost.bloom = false;
        noPost.aoQuality = AmbientOcclusionQuality::Off;
        noPost.antiAliasing = AntiAliasing::Off;
        renderer.ApplySettings(noPost);

        Scene scene("NoPostScene");
        PopulateScene(scene, assetManager);
        const CameraRenderData camera = MakeOrbitCamera(800.0f / 600.0f);
        for (int i = 0; i < 4; ++i) {
            PumpEvents(window);
            renderer.BeginFrame();
            renderer.RenderScene(scene, camera);
            renderer.EndFrame();
        }
        HK_CHECK(renderer.GetStats().drawCalls >= 4);
        HK_CHECK(renderer.GetStats().shadowDrawCalls == 0);
    }

    // Immediate-mode debug drawing overlaid on a full-quality scene render.
    {
        RendererSettings full = renderer.GetSettings();
        full.shadowQuality = ShadowQuality::High;
        full.bloom = true;
        full.aoQuality = AmbientOcclusionQuality::High;
        full.antiAliasing = AntiAliasing::FXAA;
        renderer.ApplySettings(full);

        Scene scene("DebugScene");
        PopulateScene(scene, assetManager);
        const CameraRenderData camera = MakeOrbitCamera(800.0f / 600.0f);
        for (int i = 0; i < 4; ++i) {
            PumpEvents(window);
            renderer.BeginFrame();
            renderer.Debug().DrawBox(glm::mat4(1.0f), {1.0f, 0.0f, 0.0f, 1.0f});
            renderer.Debug().DrawSphere({0.0f, 1.0f, 0.0f}, 0.5f, {0.0f, 1.0f, 0.0f, 1.0f});
            renderer.Debug().DrawLine({-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f});
            renderer.RenderScene(scene, camera);
            renderer.EndFrame();
        }
    }

    // Offscreen viewport path: render the scene to a target, resize it, and blit
    // it to the swapchain (the editor's render-to-texture flow).
    {
        RenderTargetDesc rtDesc;
        rtDesc.width = 640;
        rtDesc.height = 360;
        rtDesc.debugName = "EditorViewport";
        const TextureHandle viewport = renderer.CreateRenderTarget(rtDesc);
        HK_CHECK(viewport.IsValid());

        Scene scene("ViewportScene");
        PopulateScene(scene, assetManager);
        for (int i = 0; i < 4; ++i) {
            PumpEvents(window);
            renderer.BeginFrame();
            renderer.RenderSceneToTarget(scene, MakeOrbitCamera(640.0f / 360.0f), viewport);
            renderer.BlitTargetToSwapchain(viewport);
            renderer.EndFrame();
        }
        HK_CHECK(renderer.GetStats().drawCalls >= 4);

        renderer.ResizeRenderTarget(viewport, 1024, 576);
        for (int i = 0; i < 4; ++i) {
            PumpEvents(window);
            renderer.BeginFrame();
            renderer.RenderSceneToTarget(scene, MakeOrbitCamera(1024.0f / 576.0f), viewport);
            renderer.BlitTargetToSwapchain(viewport);
            renderer.EndFrame();
        }
        HK_CHECK(renderer.GetStats().triangleCount > 0);
    }

    // Stats should be populated (VRAM budget known on a real device).
    HK_CHECK(renderer.GetStats().budgetVRAMBytes > 0);

    renderer.Shutdown();
    assetManager.Shutdown();
    HK_CHECK(!renderer.IsInitialized());

    window.Destroy();
    SDL_Quit();
    HK_TEST_INFO("RendererSmokeTests completed");
}
