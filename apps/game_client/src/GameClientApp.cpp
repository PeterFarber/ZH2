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
#include "Hockey/Gameplay/Simulation/GameplaySnapshot.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"
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
#include "Hockey/UI/ClientFlowSerializer.hpp"
#include "Hockey/UI/UISettings.hpp"
#include "Hockey/UI/ViewModels.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>

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

Hockey::Entity FindLocalGameplayPlayer(Hockey::Scene& scene, uint32_t playerIndex) {
    auto view = scene.Registry().view<Hockey::PlayerComponent>();
    for (const entt::entity handle : view) {
        Hockey::Entity player(handle, &scene);
        if (player.GetComponent<Hockey::PlayerComponent>().playerIndex == playerIndex) {
            return player;
        }
    }
    return {};
}

bool PlayerHasPuck(Hockey::Scene& scene, Hockey::Entity player) {
    if (!player.IsValid()) {
        return false;
    }
    if (player.HasComponent<Hockey::SkaterComponent>() && player.GetComponent<Hockey::SkaterComponent>().hasPuck) {
        return true;
    }

    auto pucks = scene.Registry().view<Hockey::PuckGameplayComponent>();
    for (const entt::entity handle : pucks) {
        Hockey::Entity puck(handle, &scene);
        if (puck.GetComponent<Hockey::PuckGameplayComponent>().possessingPlayer == player.GetUUID()) {
            return true;
        }
    }
    return false;
}

std::string RuntimePhaseLabel(Hockey::MatchPhase phase) {
    switch (phase) {
    case Hockey::MatchPhase::NotStarted: return "Not started";
    case Hockey::MatchPhase::Warmup: return "Warmup";
    case Hockey::MatchPhase::FaceoffSetup: return "Faceoff setup";
    case Hockey::MatchPhase::Faceoff: return "Faceoff";
    case Hockey::MatchPhase::Playing: return "Playing";
    case Hockey::MatchPhase::GoalScored: return "Goal";
    case Hockey::MatchPhase::PeriodEnded: return "Period ended";
    case Hockey::MatchPhase::MatchEnded: return "Final";
    case Hockey::MatchPhase::Paused: return "Paused";
    }
    return "Not started";
}

Hockey::HudViewModel BuildRuntimeHudViewModel(const Hockey::GameplaySnapshot& snapshot, uint32_t localPlayerIndex) {
    Hockey::HudViewModel hud;
    hud.period = static_cast<int>(snapshot.match.period);
    hud.homeScore = static_cast<int>(snapshot.match.homeScore);
    hud.awayScore = static_cast<int>(snapshot.match.awayScore);
    hud.clockText = Hockey::FormatClockText(snapshot.match.periodTimeRemaining);
    hud.phaseLabel = RuntimePhaseLabel(snapshot.match.phase);
    hud.shotChargeRatio = 0.0f;
    hud.possessionLabel = snapshot.puck.possessingPlayer.IsValid() ? "Possessed" : "Loose puck";

    for (const Hockey::PlayerGameplaySnapshot& player : snapshot.players) {
        const std::string playerLabel = std::string(Hockey::GameplayTeamToString(player.team)) + " " +
                                        Hockey::GameplayRoleToString(player.role) + " " +
                                        std::to_string(player.playerIndex + 1);
        if (player.playerIndex == localPlayerIndex) {
            hud.localPlayerLabel = playerLabel;
            hud.shotChargeRatio = player.shotChargeRatio;
        }
        if (snapshot.puck.possessingPlayer == player.entity || player.hasPuck) {
            hud.possessionLabel = playerLabel;
        }
    }

    if (hud.localPlayerLabel.empty()) {
        hud.localPlayerLabel = "Player " + std::to_string(localPlayerIndex + 1);
    }
    return hud;
}

void ApplyHudViewModel(Hockey::RmlUiContext& ui, const Hockey::HudViewModel& hud) {
    ui.SetElementText("home-score", std::to_string(hud.homeScore));
    ui.SetElementText("away-score", std::to_string(hud.awayScore));
    ui.SetElementText("match-clock", hud.clockText);
    ui.SetElementText("period-label", "P" + std::to_string(hud.period));
    ui.SetElementText("phase-label", hud.phaseLabel);
    ui.SetElementText("local-player-label", hud.localPlayerLabel);
    ui.SetElementText("possession-label", hud.possessionLabel);
    ui.SetElementText("shot-charge", Hockey::FormatShotCharge(hud.shotChargeRatio));
    ui.SetElementText("hud-notification", hud.notificationText);
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
    m_UISettings = Hockey::LoadUISettings(m_Config);
    m_UIEnabled = m_UISettings.enabled && !cmd.Has("--no-ui");
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
    if (const Hockey::Result<Hockey::GameplayTuning> loaded =
            Hockey::TuningSerializer::Load(Hockey::Paths::DataFile("gameplay/tuning.default.yaml"))) {
        m_GameplayTuning = loaded.value;
    } else {
        HK_CLIENT_INFO("Gameplay tuning load failed: {}. Using built-in defaults.", loaded.error);
    }
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
        const Hockey::Status status =
            m_GameplayWorld.Init(m_Scene, physicsWorld, m_GameplaySettings, m_GameplayTuning);
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

    if (m_UIEnabled) {
        Hockey::ClientFlow flow = Hockey::MakeDefaultClientFlow();
        const std::filesystem::path flowPath = Hockey::Paths::Resolve(m_UISettings.startFlow);
        if (const Hockey::Result<Hockey::ClientFlow> loaded = Hockey::ClientFlowSerializer::Load(flowPath)) {
            flow = loaded.value;
        } else {
            HK_CLIENT_INFO("Client flow load failed: {}. Using built-in default flow.", loaded.error);
        }
        m_ClientFlow.SetFlow(flow);
        m_ClientFlow.Boot();

        Hockey::RmlUiContextDesc uiDesc;
        uiDesc.renderer = &m_Renderer;
        uiDesc.root = Hockey::Paths::Get().root;
        uiDesc.settings = m_UISettings;
        uiDesc.name = "game-client-ui";
        uiDesc.width = GetWindow().Width();
        uiDesc.height = GetWindow().Height();
        if (!m_UIContext.Initialize(uiDesc)) {
            HK_CLIENT_INFO("Runtime UI disabled: RmlUi context initialization failed");
            m_UIEnabled = false;
        } else if (!LoadRuntimeUIScreen()) {
            HK_CLIENT_INFO("Runtime UI disabled: could not load '{}'",
                           m_ClientFlow.Flow().ScreenDocument(m_ClientFlow.ActiveScreen()));
            m_UIContext.Shutdown();
            m_UIEnabled = false;
        } else {
            HK_CLIENT_INFO("Runtime UI booted to '{}' from '{}'", Hockey::ToString(m_ClientFlow.ActiveScreen()),
                           flowPath.string());
        }
    }
    m_LastWidth = GetWindow().Width();
    m_LastHeight = GetWindow().Height();
    return true;
}

bool GameClientApp::LoadRuntimeUIScreen() {
    if (!m_UIContext.IsInitialized()) {
        return false;
    }

    m_UIContext.UnloadAllDocuments();
    m_UIContext.Update();
    if (!m_UIContext.LoadDocument(m_ClientFlow.Flow().ScreenDocument(m_ClientFlow.ActiveScreen()))) {
        return false;
    }
    BindRuntimeUIActions();
    return true;
}

void GameClientApp::BindRuntimeUIActions() {
    m_UIContext.BindClickAction("play-offline", [this]() { QueueRuntimeUIAction(Hockey::UIAction::StartOffline); });
    m_UIContext.BindClickAction("open-settings", [this]() { QueueRuntimeUIAction(Hockey::UIAction::OpenSettings); });
    m_UIContext.BindClickAction("open-lobby", [this]() { QueueRuntimeUIAction(Hockey::UIAction::OpenLobby); });
    m_UIContext.BindClickAction("quit-game", [this]() { QueueRuntimeUIAction(Hockey::UIAction::Quit); });
    m_UIContext.BindClickAction("settings-back", [this]() { QueueRuntimeUIAction(Hockey::UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("lobby-back", [this]() { QueueRuntimeUIAction(Hockey::UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("resume-game", [this]() { QueueRuntimeUIAction(Hockey::UIAction::Resume); });
    m_UIContext.BindClickAction("return-to-menu", [this]() { QueueRuntimeUIAction(Hockey::UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("pause-quit", [this]() { QueueRuntimeUIAction(Hockey::UIAction::Quit); });
    m_UIContext.BindClickAction("scoreboard-close", [this]() { QueueRuntimeUIAction(Hockey::UIAction::Resume); });
    m_UIContext.BindClickAction("end-return-to-menu", [this]() { QueueRuntimeUIAction(Hockey::UIAction::ReturnToMenu); });
}

void GameClientApp::QueueRuntimeUIAction(Hockey::UIAction action) {
    if (action == Hockey::UIAction::Quit) {
        RequestQuit();
        return;
    }

    m_ClientFlow.Dispatch(action);
    if (action == Hockey::UIAction::StartOffline) {
        const std::optional<std::string> requestedScene = m_ClientFlow.TakeRequestedGameplayScene();
        if (requestedScene.has_value() && !requestedScene->empty() && !LoadOfflineGameplayScene(*requestedScene)) {
            m_ClientFlow.Dispatch(Hockey::UIAction::ReturnToMenu);
            m_UIReloadRequested = true;
            return;
        }
        m_ClientFlow.Dispatch(Hockey::UIAction::Resume);
    }
    m_UIReloadRequested = true;
}

bool GameClientApp::LoadOfflineGameplayScene(const std::string& scenePath) {
    const std::filesystem::path resolved = Hockey::Paths::Resolve(scenePath);
    if (!Hockey::FileSystem::Exists(resolved)) {
        HK_CLIENT_INFO("Client flow offline scene '{}' not found", resolved.string());
        return false;
    }

    m_GameplayWorld.Shutdown();
    if (m_PhysicsReady) {
        m_Scene.OnSimulationStop();
        m_Scene.ClearSystems();
        m_PhysicsSystem = nullptr;
        m_PhysicsReady = false;
    }

    Hockey::SceneSerializer serializer(m_Scene);
    if (!serializer.Deserialize(resolved)) {
        HK_CLIENT_INFO("Client flow offline scene '{}' failed to load", resolved.string());
        return false;
    }
    m_Scene.SetMode(Hockey::SceneMode::Play);
    EnsureRenderables(m_Scene);

    if (m_Config.GetBool("physics.enabled", true) && Hockey::Physics::IsInitialized()) {
        Hockey::PhysicsSettings physicsSettings;
        Hockey::LoadPhysicsSettings(m_Config, physicsSettings);
        m_PhysicsDebug = physicsSettings.enableDebugDraw || GetCommandLine().Has("--physics-debug");
        if (!m_LocalGameplayEnabled && physicsSettings.fixedDeltaSeconds > 0.0f) {
            m_SimulationTimestep.SetTickRate(1.0 / static_cast<double>(physicsSettings.fixedDeltaSeconds));
        }

        auto physicsSystem = std::make_unique<Hockey::PhysicsSystem>(physicsSettings);
        m_PhysicsSystem = physicsSystem.get();
        m_Scene.AddSystem(std::move(physicsSystem));
        m_Scene.OnSimulationStart();
        m_PhysicsReady = true;
    }

    m_LocalGameplayEnabled = m_GameplaySettings.enabled && m_Config.GetBool("gameplay.local_play", true);
    if (m_LocalGameplayEnabled) {
        Hockey::PhysicsWorld* physicsWorld = m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr;
        const Hockey::Status status =
            m_GameplayWorld.Init(m_Scene, physicsWorld, m_GameplaySettings, m_GameplayTuning);
        if (!status) {
            m_LocalGameplayEnabled = false;
            HK_CLIENT_INFO("Offline gameplay disabled for '{}': {}", resolved.string(), status.error);
        }
    }

    m_SimulationTimestep.Reset();
    m_LocalInputSequence = 0;
    HK_CLIENT_INFO("Client flow loaded offline scene '{}'", resolved.string());
    return true;
}

void GameClientApp::OnShutdown() {
    HK_CLIENT_INFO("Shutdown");
    if (m_UIContext.IsInitialized()) {
        m_UIContext.Shutdown();
    }
    m_UIEnabled = false;
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
    if (m_UIEnabled &&
        (m_ClientFlow.ActiveScreen() != Hockey::UIScreenId::MatchHud || m_UIInput.WantsMouseCapture() ||
         m_UIInput.WantsKeyboardCapture())) {
        return input;
    }
    const Hockey::Entity localPlayer = FindLocalGameplayPlayer(m_Scene, input.playerIndex);
    const bool localHasPuck = PlayerHasPuck(m_Scene, localPlayer);
    const bool localIsGoalie = localPlayer.IsValid() && localPlayer.HasComponent<Hockey::GoalieComponent>();

    if (Hockey::Input::WasMouseButtonPressed(Hockey::MouseButton::Right)) {
        glm::vec3 target{0.0f};
        if (ProjectMouseToIcePlane(m_Scene, GetWindow().Width(), GetWindow().Height(), target)) {
            input.moveTarget = target;
            input.setMoveTarget = true;
        }
    }

    input.move.x = (Hockey::Input::IsKeyDown(Hockey::KeyCode::D) ? 1.0f : 0.0f) -
                   (Hockey::Input::IsKeyDown(Hockey::KeyCode::A) ? 1.0f : 0.0f);
    input.move.y = Hockey::Input::IsKeyDown(Hockey::KeyCode::W) ? 1.0f : 0.0f;
    if (glm::dot(input.move, input.move) > 1.0f) {
        input.move = glm::normalize(input.move);
    }

    input.aim.x = (Hockey::Input::IsKeyDown(Hockey::KeyCode::Right) ? 1.0f : 0.0f) -
                  (Hockey::Input::IsKeyDown(Hockey::KeyCode::Left) ? 1.0f : 0.0f);
    input.aim.y = (Hockey::Input::IsKeyDown(Hockey::KeyCode::Up) ? 1.0f : 0.0f) -
                  (Hockey::Input::IsKeyDown(Hockey::KeyCode::Down) ? 1.0f : 0.0f);
    if (glm::dot(input.aim, input.aim) > 1.0f) {
        input.aim = glm::normalize(input.aim);
    }

    input.boostPressed = Hockey::Input::WasKeyPressed(Hockey::KeyCode::Z);
    input.brakePressed = Hockey::Input::WasKeyPressed(Hockey::KeyCode::S);
    input.brakeHeld = Hockey::Input::IsKeyDown(Hockey::KeyCode::S);
    if (input.brakePressed) {
        input.clearMoveTarget = true;
    }

    const bool xPressed = Hockey::Input::WasKeyPressed(Hockey::KeyCode::X);
    input.quickTurnPressed = xPressed && !localIsGoalie;
    input.goalieShieldPressed = xPressed && localIsGoalie;

    const bool leftPressed = Hockey::Input::WasMouseButtonPressed(Hockey::MouseButton::Left);
    const bool leftHeld = Hockey::Input::IsMouseButtonDown(Hockey::MouseButton::Left);
    const bool leftReleased = Hockey::Input::WasMouseButtonReleased(Hockey::MouseButton::Left);
    if (localHasPuck) {
        if ((leftPressed || leftHeld || leftReleased) && localPlayer.HasComponent<Hockey::TransformComponent>()) {
            glm::vec3 aimTarget{0.0f};
            glm::vec2 mouseAim{0.0f};
            if (ProjectMouseToIcePlane(m_Scene, GetWindow().Width(), GetWindow().Height(), aimTarget) &&
                Hockey::TryBuildAimFromWorldTarget(localPlayer.GetComponent<Hockey::TransformComponent>().localPosition,
                                                   aimTarget, mouseAim)) {
                input.aim = mouseAim;
            }
        }
        input.shootPressed = leftPressed;
        input.shootHeld = leftHeld;
        input.shootReleased = leftReleased;
    } else {
        input.stealPressed = leftPressed;
    }

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
        if (m_LocalGameplayEnabled && m_GameplayWorld.IsInitialized()) {
            m_GameplayWorld.SyncPhysicsState(m_Scene);
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

void GameClientApp::SubmitGameplayDebugDraw() {
    if (!m_LocalGameplayEnabled || !m_GameplayWorld.IsInitialized()) {
        return;
    }

    const Hockey::GameplaySnapshot snapshot =
        Hockey::BuildGameplaySnapshot(m_Scene, m_SimulationTimestep.GetTick(), m_GameplayWorld.GetTuning());

    const Hockey::PlayerGameplaySnapshot* localPlayer = nullptr;
    for (const Hockey::PlayerGameplaySnapshot& player : snapshot.players) {
        if (player.playerIndex == 0) {
            localPlayer = &player;
            break;
        }
    }
    if (localPlayer == nullptr || !localPlayer->shotCharging) {
        return;
    }

    const float ratio = std::clamp(localPlayer->shotChargeRatio, 0.0f, 1.0f);
    const glm::vec3 center = localPlayer->position + glm::vec3{0.0f, 2.2f, 0.0f};
    constexpr float kWidth = 1.35f;
    constexpr float kHeight = 0.16f;
    const glm::vec3 left = center + glm::vec3{-kWidth * 0.5f, 0.0f, 0.0f};
    const glm::vec3 right = center + glm::vec3{kWidth * 0.5f, 0.0f, 0.0f};
    const glm::vec3 top{0.0f, kHeight * 0.5f, 0.0f};
    const glm::vec3 bottom{0.0f, -kHeight * 0.5f, 0.0f};

    Hockey::DebugDraw& debug = m_Renderer.Debug();
    const glm::vec4 outline{0.02f, 0.02f, 0.025f, 1.0f};
    const glm::vec4 track{0.18f, 0.20f, 0.23f, 1.0f};
    const glm::vec4 fill = ratio >= 0.95f ? glm::vec4{1.0f, 0.82f, 0.18f, 1.0f}
                                           : glm::vec4{0.30f, 0.72f, 1.0f, 1.0f};
    debug.DrawLine(left + top, right + top, outline);
    debug.DrawLine(left + bottom, right + bottom, outline);
    debug.DrawLine(left + bottom, left + top, outline);
    debug.DrawLine(right + bottom, right + top, outline);
    debug.DrawLine(left, right, track);

    const glm::vec3 fillRight = left + (right - left) * ratio;
    debug.DrawLine(left + glm::vec3{0.0f, -0.035f, 0.0f}, fillRight + glm::vec3{0.0f, -0.035f, 0.0f}, fill);
    debug.DrawLine(left, fillRight, fill);
    debug.DrawLine(left + glm::vec3{0.0f, 0.035f, 0.0f}, fillRight + glm::vec3{0.0f, 0.035f, 0.0f}, fill);
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
        if (m_UIEnabled && m_UIContext.IsInitialized()) {
            m_UIContext.SetDimensions(width, height);
        }
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
    SubmitGameplayDebugDraw();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    Hockey::CameraRenderData camera;
    if (Hockey::FindActiveCamera(m_Scene, aspect, camera)) {
        m_Renderer.RenderScene(m_Scene, camera);
    }
    if (m_UIEnabled && m_UIContext.IsInitialized()) {
        if (m_UIReloadRequested) {
            if (!LoadRuntimeUIScreen()) {
                HK_CLIENT_INFO("Runtime UI screen reload failed for '{}'",
                               m_ClientFlow.Flow().ScreenDocument(m_ClientFlow.ActiveScreen()));
                m_UIContext.Shutdown();
                m_UIEnabled = false;
            }
            m_UIReloadRequested = false;
        }
        if (m_ClientFlow.ActiveScreen() == Hockey::UIScreenId::MatchHud && m_LocalGameplayEnabled &&
            m_GameplayWorld.IsInitialized()) {
            const Hockey::GameplaySnapshot snapshot = Hockey::BuildGameplaySnapshot(
                m_Scene, m_SimulationTimestep.GetTick(), m_GameplayWorld.GetTuning());
            ApplyHudViewModel(m_UIContext, BuildRuntimeHudViewModel(snapshot, 0));
        }
        m_UIContext.Update();
        m_UIContext.Render();
    }
    m_Renderer.EndFrame();
}

void GameClientApp::OnEvent(const Hockey::Event& event) {
    const bool isEscape = event.type == Hockey::EventType::KeyPressed &&
                          Hockey::Keyboard::FromScancode(event.key) == Hockey::KeyCode::Escape;
    if (m_UIEnabled && m_UIContext.IsInitialized()) {
        if (event.type == Hockey::EventType::WindowResize) {
            m_UIContext.SetDimensions(event.windowWidth, event.windowHeight);
        } else {
            m_UIInput.ProcessEvent(*m_UIContext.RawContext(), event);
        }
        if (isEscape) {
            if (m_ClientFlow.ActiveScreen() == Hockey::UIScreenId::MatchHud) {
                QueueRuntimeUIAction(Hockey::UIAction::Pause);
            } else if (m_ClientFlow.ActiveScreen() == Hockey::UIScreenId::PauseMenu) {
                QueueRuntimeUIAction(Hockey::UIAction::Resume);
            }
            return;
        }
    }
    if (isEscape && !m_UIInput.WantsKeyboardCapture()) {
        RequestQuit();
    }
}
