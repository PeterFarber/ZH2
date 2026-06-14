#include "GameClientApp.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Physics/PhysicsSystem.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/RendererInitInfo.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"
#include <filesystem>
#include <memory>

namespace {

// Adds a camera, directional light, and ambient environment to a scene that is
// missing them, so a freshly loaded or empty scene still renders something.
void EnsureRenderables(Hockey::Scene& scene) {
    using namespace Hockey;
    bool hasCamera = false;
    bool hasLight = false;
    bool hasEnv = false;
    for (const entt::entity handle : scene.Registry().view<CameraComponent>()) {
        (void)handle;
        hasCamera = true;
    }
    for (const entt::entity handle : scene.Registry().view<LightComponent>()) {
        (void)handle;
        hasLight = true;
    }
    for (const entt::entity handle : scene.Registry().view<EnvironmentComponent>()) {
        (void)handle;
        hasEnv = true;
    }

    if (!hasCamera) {
        Entity camera = scene.CreateEntity("Main Camera");
        auto& transform = camera.GetComponent<TransformComponent>();
        transform.localPosition = {0.0f, 4.0f, 12.0f};
        transform.localRotation = {-15.0f, 0.0f, 0.0f};
        auto& cam = camera.AddComponent<CameraComponent>();
        cam.primary = true;
    }
    if (!hasLight) {
        Entity sun = scene.CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().localRotation = {-50.0f, -30.0f, 0.0f};
        auto& light = sun.AddComponent<LightComponent>();
        light.type = LightComponent::Type::Directional;
        light.color = {1.0f, 0.97f, 0.9f};
        light.intensity = 3.0f;
    }
    if (!hasEnv) {
        Entity env = scene.CreateEntity("Environment");
        auto& environment = env.AddComponent<EnvironmentComponent>();
        environment.ambientColor = {0.05f, 0.06f, 0.08f};
        environment.ambientIntensity = 1.0f;
    }
}

} // namespace

bool GameClientApp::OnInit() {
    auto root = GetCommandLine().GetString("--root", ".");
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    Hockey::Log::Init(Hockey::Paths::LogFile("client.log"));
    Hockey::CrashHandler::Install();
    Hockey::JobSystem::Init();
    m_Config.Load(Hockey::Paths::ConfigFile("client.toml"));
    if (!CreateAppWindowFromConfig(m_Config)) {
        HK_CORE_ERROR("Failed to create game client window");
        return false;
    }

    HK_CLIENT_INFO("Game client started on {} ({} hardware threads)", Hockey::Platform::OSName(),
                   Hockey::Platform::HardwareThreadCount());

    const auto gamepads = Hockey::Input::ConnectedGamepads();
    HK_CLIENT_INFO("Connected gamepads: {}", gamepads.size());
    for (const auto& gamepad : gamepads) {
        HK_CLIENT_INFO("  Gamepad [{}]: {}", gamepad.InstanceId(), gamepad.Name());
    }

    // Register physics component serialization + materials before scene load so
    // physics components deserialize and resolve their materials.
    Hockey::RegisterPhysicsComponents();
    Hockey::PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    m_Scene.SetMode(Hockey::SceneMode::Play);
    const std::string startupScene = m_Config.GetString("scene.startup_scene", "");
    if (!startupScene.empty()) {
        const std::filesystem::path scenePath = Hockey::Paths::Get().root / startupScene;
        if (Hockey::FileSystem::Exists(scenePath)) {
            Hockey::SceneSerializer serializer(m_Scene);
            if (!serializer.Deserialize(scenePath)) {
                HK_CLIENT_INFO("Could not load startup scene; using empty Play scene");
            }
        } else {
            HK_CLIENT_INFO("Startup scene '{}' not found; using empty Play scene", scenePath.string());
        }
    }

    EnsureRenderables(m_Scene);
    HK_CLIENT_INFO("Active scene '{}' with {} entities", m_Scene.GetName(), m_Scene.EntityCount());

    if (m_Config.GetBool("physics.enabled", true)) {
        if (!Hockey::Physics::Init()) {
            HK_CLIENT_INFO("Physics failed to initialise; running without physics");
        } else {
            Hockey::PhysicsSettings physicsSettings;
            Hockey::LoadPhysicsSettings(m_Config, physicsSettings);
            m_PhysicsDebug = physicsSettings.enableDebugDraw || GetCommandLine().Has("--physics-debug");
            if (physicsSettings.fixedDeltaSeconds > 0.0f) {
                m_PhysicsTimestep.SetTickRate(1.0 / static_cast<double>(physicsSettings.fixedDeltaSeconds));
            }

            auto physicsSystem = std::make_unique<Hockey::PhysicsSystem>(physicsSettings);
            m_PhysicsSystem = physicsSystem.get();
            m_Scene.AddSystem(std::move(physicsSystem));
            m_Scene.OnSimulationStart();
            m_PhysicsReady = true;
            HK_CLIENT_INFO("Physics enabled ({} bodies, debug draw {})", m_PhysicsSystem->World().BodyCount(),
                           m_PhysicsDebug ? "on" : "off");
        }
    }

    Hockey::RendererInitInfo rendererInfo;
    rendererInfo.window = &GetWindow();
    rendererInfo.settings = Hockey::MakeDefaultRendererSettings();
    Hockey::LoadRendererSettings(m_Config, rendererInfo.settings);
    rendererInfo.enableValidation = GetCommandLine().Has("--vk-validation");
    rendererInfo.shaderSourceDirectory = Hockey::Paths::Get().root / "data/shaders/src";
    rendererInfo.shaderBinaryDirectory = Hockey::Paths::Get().root / "data/shaders/bin";
    if (const Hockey::Status status = m_Renderer.Init(rendererInfo); !status) {
        HK_CORE_ERROR("Renderer init failed: {}", status.error);
        return false;
    }
    m_RendererReady = true;

    // Content pipeline (load-only on the client): load the asset database so the
    // renderer can resolve MeshRendererComponent asset ids to cooked GPU
    // resources. Cooking is owned by the editor/asset tool, not the client.
    if (const Hockey::Status status =
            m_AssetManager.Init(Hockey::AssetManager::DefaultCreateInfo(Hockey::Paths::Get().root));
        status) {
        m_AssetsReady = true;
        m_Renderer.SetAssetManager(&m_AssetManager);
    } else {
        HK_CLIENT_INFO("Asset manager unavailable: {}", status.error);
    }
    m_LastWidth = GetWindow().Width();
    m_LastHeight = GetWindow().Height();
    return true;
}

void GameClientApp::OnShutdown() {
    HK_CLIENT_INFO("Shutdown");
    if (m_PhysicsReady) {
        m_Scene.OnSimulationStop();
        m_Scene.ClearSystems(); // destroy physics world before global Jolt teardown
        m_PhysicsSystem = nullptr;
        m_PhysicsReady = false;
        Hockey::Physics::Shutdown();
    }
    if (m_RendererReady) {
        m_Renderer.WaitIdle();
        m_Renderer.SetAssetManager(nullptr);
    }
    if (m_AssetsReady) {
        m_AssetManager.Shutdown();
        m_AssetsReady = false;
    }
    m_Renderer.Shutdown();
    m_RendererReady = false;
    Hockey::JobSystem::Shutdown();
    Hockey::Log::Shutdown();
}

void GameClientApp::StepPhysics(float deltaTime) {
    if (!m_PhysicsReady) {
        return;
    }
    const int steps = m_PhysicsTimestep.Accumulate(static_cast<double>(deltaTime));
    const float fixedDelta = static_cast<float>(m_PhysicsTimestep.GetFixedDeltaSeconds());
    for (int i = 0; i < steps; ++i) {
        m_Scene.OnFixedUpdate(fixedDelta);
        m_PhysicsTimestep.AdvanceTick();
    }
    // No hockey gameplay rules yet: consume events so the queues stay bounded.
    m_PhysicsSystem->World().DrainContactEvents();
    m_PhysicsSystem->World().DrainTriggerEvents();
}

void GameClientApp::SubmitPhysicsDebugDraw() {
    if (!m_PhysicsReady || !m_PhysicsDebug) {
        return;
    }
    Hockey::PhysicsDebugDrawList list;
    m_PhysicsSystem->World().CollectDebugDraw(list);
    Hockey::DebugDraw& debug = m_Renderer.Debug();
    for (const Hockey::PhysicsDebugLine& line : list.lines) {
        debug.DrawLine(line.a, line.b, line.color);
    }
}

void GameClientApp::OnUpdate(float deltaTime) {
    m_Scene.OnUpdate(deltaTime);
    StepPhysics(deltaTime);
    if (!m_RendererReady) {
        return;
    }

    const uint32_t width = GetWindow().Width();
    const uint32_t height = GetWindow().Height();
    if (width != m_LastWidth || height != m_LastHeight) {
        m_Renderer.Resize(width, height);
        m_LastWidth = width;
        m_LastHeight = height;
    }

    m_Renderer.BeginFrame();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    Hockey::CameraRenderData camera;
    if (Hockey::FindActiveCamera(m_Scene, aspect, camera)) {
        m_Renderer.RenderScene(m_Scene, camera);
    }
    SubmitPhysicsDebugDraw();
    m_Renderer.EndFrame();
}

void GameClientApp::OnEvent(const Hockey::Event& event) {
    if (event.type == Hockey::EventType::KeyPressed &&
        Hockey::Keyboard::FromScancode(event.key) == Hockey::KeyCode::Escape) {
        RequestQuit();
    }
}
