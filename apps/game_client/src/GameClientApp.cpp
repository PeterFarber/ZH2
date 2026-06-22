#include "GameClientApp.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/Screenshot.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Gameplay/Gameplay.hpp"
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
#include <cmath>
#include <filesystem>
#include <memory>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

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

bool ProjectMouseToIcePlane(Hockey::Scene& scene, std::uint32_t width, std::uint32_t height, glm::vec3& outPoint) {
    if (width == 0 || height == 0) {
        return false;
    }

    Hockey::CameraRenderData camera;
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    if (!Hockey::FindActiveCamera(scene, aspect, camera)) {
        return false;
    }

    const float x = (Hockey::Input::MouseX() / static_cast<float>(width)) * 2.0f - 1.0f;
    const float y = (Hockey::Input::MouseY() / static_cast<float>(height)) * 2.0f - 1.0f;
    const glm::mat4 invViewProj = glm::inverse(camera.projection * camera.view);
    const glm::vec4 nearClip = invViewProj * glm::vec4{x, y, 0.0f, 1.0f};
    const glm::vec4 farClip = invViewProj * glm::vec4{x, y, 1.0f, 1.0f};
    const glm::vec3 nearWorld = glm::vec3(nearClip) / nearClip.w;
    const glm::vec3 farWorld = glm::vec3(farClip) / farClip.w;
    const glm::vec3 dir = glm::normalize(farWorld - nearWorld);
    if (std::abs(dir.y) < 1e-5f) {
        return false;
    }
    const float t = -nearWorld.y / dir.y;
    if (t < 0.0f) {
        return false;
    }
    outPoint = nearWorld + dir * t;
    return true;
}

} // namespace

bool GameClientApp::OnInit() {
    const auto& cmd = GetCommandLine();
    const std::filesystem::path root =
        cmd.Has("--root") ? std::filesystem::path(cmd.GetString("--root", "")) : std::filesystem::path{};
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    m_AutoScreenshotPending = cmd.Has("--screenshot") || cmd.Has("--screenshot-prefix");
    m_ScreenshotPrefix = cmd.GetString("--screenshot-prefix", cmd.GetString("--screenshot", "game"));
    if (m_ScreenshotPrefix.empty() || m_ScreenshotPrefix == "true") {
        m_ScreenshotPrefix = "game";
    }

    const auto logPath = cmd.Has("--log") ? Hockey::Paths::Resolve(cmd.GetString("--log"))
                                          : Hockey::Paths::LogFile("client.log");
    Hockey::Log::Init(logPath);
    Hockey::CrashHandler::Install();
    Hockey::JobSystem::Init();
    const auto configPath = cmd.Has("--config") ? Hockey::Paths::Resolve(cmd.GetString("--config"))
                                                : Hockey::Paths::ConfigFile("client.toml");
    m_Config.Load(configPath);
    // Honor the input.gamepad_enabled config key (closes pads opened at init).
    Hockey::Input::SetGamepadEnabled(m_Config.GetBool("input.gamepad_enabled", true));
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
    Hockey::RegisterGameplayComponents();
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

    m_GameplaySettings = Hockey::LoadGameplaySettings(m_Config);
    m_LocalGameplayEnabled = m_GameplaySettings.enabled && m_Config.GetBool("gameplay.local_play", true);
    if (m_GameplaySettings.fixedDeltaSeconds > 0.0f) {
        m_SimulationTimestep.SetTickRate(1.0 / static_cast<double>(m_GameplaySettings.fixedDeltaSeconds));
    }

    if (m_Config.GetBool("physics.enabled", true)) {
        if (!Hockey::Physics::Init()) {
            HK_CLIENT_INFO("Physics failed to initialise; running without physics");
        } else {
            Hockey::PhysicsSettings physicsSettings;
            Hockey::LoadPhysicsSettings(m_Config, physicsSettings);
            m_PhysicsDebug = physicsSettings.enableDebugDraw || GetCommandLine().Has("--physics-debug");
            if (!m_LocalGameplayEnabled && physicsSettings.fixedDeltaSeconds > 0.0f) {
                m_SimulationTimestep.SetTickRate(1.0 / static_cast<double>(physicsSettings.fixedDeltaSeconds));
            }

            auto physicsSystem = std::make_unique<Hockey::PhysicsSystem>(physicsSettings);
            m_PhysicsSystem = physicsSystem.get();
            m_Scene.AddSystem(std::move(physicsSystem));
            // The client plays scenes rather than editing them; switch out of Edit
            // mode so the physics system actually simulates (ShouldSimulate gates
            // on SceneMode::Edit).
            m_Scene.SetMode(Hockey::SceneMode::Play);
            m_Scene.OnSimulationStart();
            m_PhysicsReady = true;
            HK_CLIENT_INFO("Physics enabled ({} bodies, debug draw {})", m_PhysicsSystem->World().BodyCount(),
                           m_PhysicsDebug ? "on" : "off");
        }
    }

    if (m_LocalGameplayEnabled) {
        Hockey::PhysicsWorld* physicsWorld = m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr;
        const Hockey::Status status = m_GameplayWorld.Init(m_Scene, physicsWorld, m_GameplaySettings);
        if (status) {
            HK_CLIENT_INFO("Local gameplay enabled");
        } else {
            m_LocalGameplayEnabled = false;
            HK_CLIENT_INFO("Local gameplay disabled: {}", status.error);
        }
    } else {
        HK_CLIENT_INFO("Local gameplay disabled by config");
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
    m_GameplayWorld.Shutdown();
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
    Hockey::CrashHandler::Shutdown();
    Hockey::Log::Shutdown();
}

Hockey::GameplayInputFrame GameClientApp::BuildLocalInput(uint64_t simulationTick) {
    Hockey::GameplayInputFrame input;
    input.playerIndex = 0;
    input.inputSequence = ++m_LocalInputSequence;
    input.simulationTick = simulationTick;

    if (Hockey::Input::WasMouseButtonPressed(Hockey::MouseButton::Right)) {
        glm::vec3 target{0.0f};
        if (ProjectMouseToIcePlane(m_Scene, GetWindow().Width(), GetWindow().Height(), target)) {
            m_LocalMoveTarget = target;
            m_HasLocalMoveTarget = true;
        }
    }

    if (m_HasLocalMoveTarget) {
        input.moveTarget = m_LocalMoveTarget;
        input.hasMoveTarget = true;
    }

    input.aim.x = (Hockey::Input::IsKeyDown(Hockey::KeyCode::Right) ? 1.0f : 0.0f) -
                  (Hockey::Input::IsKeyDown(Hockey::KeyCode::Left) ? 1.0f : 0.0f);
    input.aim.y = (Hockey::Input::IsKeyDown(Hockey::KeyCode::Up) ? 1.0f : 0.0f) -
                  (Hockey::Input::IsKeyDown(Hockey::KeyCode::Down) ? 1.0f : 0.0f);
    if (glm::dot(input.aim, input.aim) > 1.0f) {
        input.aim = glm::normalize(input.aim);
    }

    input.boostForward = Hockey::Input::IsKeyDown(Hockey::KeyCode::Z);
    input.brake = Hockey::Input::IsKeyDown(Hockey::KeyCode::S);
    input.quickTurnPressed = Hockey::Input::WasKeyPressed(Hockey::KeyCode::X);
    input.shootPressed = Hockey::Input::WasMouseButtonPressed(Hockey::MouseButton::Left);
    input.shootHeld = Hockey::Input::IsMouseButtonDown(Hockey::MouseButton::Left);
    input.shootReleased = Hockey::Input::WasMouseButtonReleased(Hockey::MouseButton::Left);
    input.pokeCheckPressed = Hockey::Input::WasMouseButtonPressed(Hockey::MouseButton::Middle);

    return input;
}

void GameClientApp::StepSimulation(float deltaTime) {
    if (!m_PhysicsReady && !m_LocalGameplayEnabled) {
        return;
    }
    const int steps = m_SimulationTimestep.Accumulate(static_cast<double>(deltaTime));
    const float fixedDelta = static_cast<float>(m_SimulationTimestep.GetFixedDeltaSeconds());
    for (int i = 0; i < steps; ++i) {
        const uint64_t tick = m_SimulationTimestep.GetTick() + 1;

        if (m_LocalGameplayEnabled && m_GameplayWorld.IsInitialized()) {
            if (Hockey::Input::WasKeyPressed(Hockey::KeyCode::R)) {
                m_GameplayWorld.ResetMatch(m_Scene);
            }
            m_GameplayWorld.PushInput(BuildLocalInput(tick));
            m_GameplayWorld.FixedUpdate(m_Scene, fixedDelta, tick);
            const std::vector<Hockey::GameplayEvent> events = m_GameplayWorld.DrainEvents();
            if (m_GameplaySettings.logGameplayEvents) {
                for (const Hockey::GameplayEvent& event : events) {
                    HK_CLIENT_INFO("Gameplay event: {}", Hockey::GameplayEventTypeToString(event.type));
                }
            }
        }

        if (m_PhysicsReady) {
            m_Scene.OnFixedUpdate(fixedDelta);
        }
        m_SimulationTimestep.AdvanceTick();
    }
    if (m_PhysicsReady) {
        m_PhysicsSystem->World().DrainContactEvents();
        m_PhysicsSystem->World().DrainTriggerEvents();
    }
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
    StepSimulation(deltaTime);
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

    auto queueScreenshot = [this](const std::string& prefix) {
        const std::filesystem::path path = Hockey::MakeScreenshotPath(prefix);
        if (const Hockey::Status status = m_Renderer.RequestScreenshot(path); !status) {
            HK_CLIENT_INFO("Screenshot request failed: {}", status.error);
        } else {
            HK_CLIENT_INFO("Screenshot queued: {}", path.string());
        }
    };

    if (m_AutoScreenshotPending) {
        queueScreenshot(m_ScreenshotPrefix);
        m_AutoScreenshotPending = false;
    }
    if (Hockey::Input::WasKeyPressed(Hockey::KeyCode::F12)) {
        queueScreenshot("game");
    }

    m_Renderer.BeginFrame();
    // Debug lines must be submitted BEFORE RenderScene: the scene pass records
    // the overlay from the accumulated line buffer, and EndFrame clears it.
    SubmitPhysicsDebugDraw();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    Hockey::CameraRenderData camera;
    if (Hockey::FindActiveCamera(m_Scene, aspect, camera)) {
        m_Renderer.RenderScene(m_Scene, camera);
    }
    m_Renderer.EndFrame();
}

void GameClientApp::OnEvent(const Hockey::Event& event) {
    if (event.type == Hockey::EventType::KeyPressed &&
        Hockey::Keyboard::FromScancode(event.key) == Hockey::KeyCode::Escape) {
        RequestQuit();
    }
}
