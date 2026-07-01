#include "GameClientApp.hpp"
#include "Hockey/Animation/AnimationSerializer.hpp"
#include "Hockey/Audio/AudioComponents.hpp"
#include "Hockey/Audio/AudioEvents.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/RuntimeConfig.hpp"
#include "Hockey/Core/RuntimeConfigDefaults.hpp"
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

struct MovementTraceEntityState {
    bool valid = false;
    Hockey::UUID entityId;
    glm::vec3 localPosition{0.0f};
    bool hasPhysicsBodyPosition = false;
    glm::vec3 physicsBodyPosition{0.0f};
    bool hasRigidBody = false;
    const char* rigidBodyType = "None";
    bool useGravity = false;
    bool allowSleeping = false;
    bool lockTranslationY = false;
    bool lockRotationX = false;
    bool lockRotationZ = false;
};

const char* RigidBodyTypeLabel(Hockey::RigidBodyType type) {
    switch (type) {
    case Hockey::RigidBodyType::Static: return "Static";
    case Hockey::RigidBodyType::Kinematic: return "Kinematic";
    case Hockey::RigidBodyType::Dynamic: return "Dynamic";
    }
    return "Unknown";
}

MovementTraceEntityState CaptureMovementTraceEntityState(Hockey::Scene& scene,
                                                         Hockey::PhysicsWorld* physicsWorld,
                                                         uint32_t playerIndex) {
    MovementTraceEntityState state;
    const Hockey::Entity player = FindLocalGameplayPlayer(scene, playerIndex);
    if (!player || !player.HasComponent<Hockey::TransformComponent>()) {
        return state;
    }

    state.valid = true;
    state.entityId = player.GetUUID();
    state.localPosition = player.GetComponent<Hockey::TransformComponent>().localPosition;
    if (physicsWorld != nullptr) {
        state.hasPhysicsBodyPosition = physicsWorld->GetBodyPosition(player, state.physicsBodyPosition);
    }
    if (player.HasComponent<Hockey::RigidBodyComponent>()) {
        const Hockey::RigidBodyComponent& body = player.GetComponent<Hockey::RigidBodyComponent>();
        state.hasRigidBody = true;
        state.rigidBodyType = RigidBodyTypeLabel(body.type);
        state.useGravity = body.useGravity;
        state.allowSleeping = body.allowSleeping;
        state.lockTranslationY = body.lockTranslationY;
        state.lockRotationX = body.lockRotationX;
        state.lockRotationZ = body.lockRotationZ;
    }
    return state;
}

Hockey::HockeyAnimationFrame BuildHockeyAnimationFrame(const Hockey::GameplaySnapshot& snapshot,
                                                       const std::vector<Hockey::GameplayEvent>& events) {
    Hockey::HockeyAnimationFrame frame;
    frame.players.reserve(snapshot.players.size());

    for (const Hockey::PlayerGameplaySnapshot& player : snapshot.players) {
        Hockey::HockeyAnimationPlayerState state;
        state.entity = player.entity;
        state.velocity = player.velocity;
        state.shotChargeRatio = player.shotChargeRatio;
        state.hasPuck = player.hasPuck;
        state.shotCharging = player.shotCharging;
        state.goalie = player.role == Hockey::GameplayRole::Goalie;
        frame.players.push_back(state);
    }

    for (const Hockey::GameplayEvent& event : events) {
        switch (event.type) {
        case Hockey::GameplayEventType::PuckShot:
            frame.triggers.push_back({event.primaryEntity, Hockey::HockeyAnimationTriggerType::Shoot});
            break;
        case Hockey::GameplayEventType::StealAttempted:
            frame.triggers.push_back({event.primaryEntity, Hockey::HockeyAnimationTriggerType::Steal});
            break;
        case Hockey::GameplayEventType::GoalieShieldStarted:
            frame.triggers.push_back({event.primaryEntity, Hockey::HockeyAnimationTriggerType::GoalieSave});
            break;
        case Hockey::GameplayEventType::GoalScored:
            if (event.team == Hockey::GameplayTeam::None) {
                frame.triggers.push_back({event.primaryEntity, Hockey::HockeyAnimationTriggerType::Celebrate});
            } else {
                for (const Hockey::PlayerGameplaySnapshot& player : snapshot.players) {
                    if (player.team == event.team) {
                        frame.triggers.push_back({player.entity, Hockey::HockeyAnimationTriggerType::Celebrate});
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    return frame;
}

bool FindActiveCameraPosition(Hockey::Scene& scene, glm::vec3& outPosition) {
    auto cameras = scene.Registry().view<Hockey::CameraComponent, Hockey::TransformComponent>();
    Hockey::Entity fallback;
    for (const entt::entity handle : cameras) {
        Hockey::Entity camera(handle, &scene);
        if (!fallback) {
            fallback = camera;
        }
        if (camera.GetComponent<Hockey::CameraComponent>().primary) {
            outPosition = glm::vec3(scene.GetWorldTransform(camera)[3]);
            return true;
        }
    }
    if (fallback) {
        outPosition = glm::vec3(scene.GetWorldTransform(fallback)[3]);
        return true;
    }
    return false;
}

bool GetPresentationSampleTrace(const HockeyClient::GameplayPresentationState& state,
                                Hockey::UUID entityId,
                                HockeyClient::GameplayPresentationState::Sample& outSample,
                                float& outDelta) {
    if (!state.GetSample(entityId, outSample) || !outSample.initialized) {
        outDelta = 0.0f;
        return false;
    }
    outDelta = glm::length(outSample.currentPosition - outSample.previousPosition);
    return true;
}

void LogMovementSmoothnessTick(uint64_t tick,
                               uint32_t playerIndex,
                               bool presentationSampleReset,
                               const MovementTraceEntityState& beforeGameplay,
                               const MovementTraceEntityState& afterGameplay,
                               const MovementTraceEntityState& afterPhysicsStep,
                               const MovementTraceEntityState& afterPhysicsSync,
                               float presentationSampleDelta) {
    if (!beforeGameplay.valid) {
        HK_CLIENT_INFO("movement_smoothness_tick tick={} playerIndex={} playerFound=false "
                       "presentationSampleReset={}",
                       tick,
                       playerIndex,
                       presentationSampleReset);
        return;
    }

    HK_CLIENT_INFO("movement_smoothness_tick tick={} playerIndex={} player={} "
                   "positionBeforeGameplay=({},{},{}) positionAfterGameplay=({},{},{}) "
                   "physicsBodyBeforeStepValid={} physicsBodyBeforeStep=({},{},{}) "
                   "positionAfterPhysicsStep=({},{},{}) physicsBodyAfterStepValid={} physicsBodyAfterStep=({},{},{}) "
                   "positionAfterPhysicsSync=({},{},{}) physicsBodyPositionValid={} physicsBodyPosition=({},{},{}) "
                   "playerRigidBodyType={} playerUseGravity={} playerAllowSleeping={} playerLockTranslationY={} "
                   "playerLockRotationX={} playerLockRotationZ={} presentationSampleReset={} presentationSampleDelta={}",
                   tick,
                   playerIndex,
                   beforeGameplay.entityId.Value(),
                   beforeGameplay.localPosition.x,
                   beforeGameplay.localPosition.y,
                   beforeGameplay.localPosition.z,
                   afterGameplay.localPosition.x,
                   afterGameplay.localPosition.y,
                   afterGameplay.localPosition.z,
                   afterGameplay.hasPhysicsBodyPosition,
                   afterGameplay.physicsBodyPosition.x,
                   afterGameplay.physicsBodyPosition.y,
                   afterGameplay.physicsBodyPosition.z,
                   afterPhysicsStep.localPosition.x,
                   afterPhysicsStep.localPosition.y,
                   afterPhysicsStep.localPosition.z,
                   afterPhysicsStep.hasPhysicsBodyPosition,
                   afterPhysicsStep.physicsBodyPosition.x,
                   afterPhysicsStep.physicsBodyPosition.y,
                   afterPhysicsStep.physicsBodyPosition.z,
                   afterPhysicsSync.localPosition.x,
                   afterPhysicsSync.localPosition.y,
                   afterPhysicsSync.localPosition.z,
                   afterPhysicsSync.hasPhysicsBodyPosition,
                   afterPhysicsSync.physicsBodyPosition.x,
                   afterPhysicsSync.physicsBodyPosition.y,
                   afterPhysicsSync.physicsBodyPosition.z,
                   afterPhysicsSync.rigidBodyType,
                   afterPhysicsSync.useGravity,
                   afterPhysicsSync.allowSleeping,
                   afterPhysicsSync.lockTranslationY,
                   afterPhysicsSync.lockRotationX,
                   afterPhysicsSync.lockRotationZ,
                   presentationSampleReset,
                   presentationSampleDelta);
}

void LogMovementSmoothnessFrame(uint64_t frameIndex,
                                float renderDeltaSeconds,
                                int fixedStepsThisFrame,
                                uint64_t tick,
                                float interpolationAlpha,
                                uint32_t playerIndex,
                                bool presentationSampleReset,
                                const MovementTraceEntityState& presentedState,
                                const HockeyClient::GameplayPresentationState::Sample& presentationSample,
                                bool hasPresentationSample,
                                float presentationSampleDelta,
                                const glm::vec3& cameraPosition,
                                bool hasCameraPosition) {
    if (!presentedState.valid) {
        HK_CLIENT_INFO("movement_smoothness_frame frameIndex={} renderDeltaSeconds={} fixedStepsThisFrame={} "
                       "tick={} interpolationAlpha={} playerIndex={} playerFound=false presentationSampleReset={}",
                       frameIndex,
                       renderDeltaSeconds,
                       fixedStepsThisFrame,
                       tick,
                       interpolationAlpha,
                       playerIndex,
                       presentationSampleReset);
        return;
    }

    const glm::vec3 rawPlayerPosition =
        hasPresentationSample ? presentationSample.currentPosition : presentedState.localPosition;
    HK_CLIENT_INFO("movement_smoothness_frame frameIndex={} renderDeltaSeconds={} fixedStepsThisFrame={} "
                   "tick={} interpolationAlpha={} playerIndex={} player={} rawPlayerPosition=({},{},{}) "
                   "presentedPlayerPosition=({},{},{}) cameraPositionValid={} cameraPosition=({},{},{}) "
                   "presentationSampleValid={} presentationSampleReset={} presentationSampleDelta={}",
                   frameIndex,
                   renderDeltaSeconds,
                   fixedStepsThisFrame,
                   tick,
                   interpolationAlpha,
                   playerIndex,
                   presentedState.entityId.Value(),
                   rawPlayerPosition.x,
                   rawPlayerPosition.y,
                   rawPlayerPosition.z,
                   presentedState.localPosition.x,
                   presentedState.localPosition.y,
                   presentedState.localPosition.z,
                   hasCameraPosition,
                   cameraPosition.x,
                   cameraPosition.y,
                   cameraPosition.z,
                   hasPresentationSample,
                   presentationSampleReset,
                   presentationSampleDelta);
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
    const Hockey::RuntimeConfigLoadInfo configInfo{
        .embeddedToml = Hockey::DefaultRuntimeConfigToml(),
        .embeddedSourceName = "built-in-runtime-defaults",
        .siblingFilename = "HockeyGameClient.toml",
        .commandLineOverride = cmd.Has("--config") ? Hockey::Paths::Resolve(cmd.GetString("--config", ""))
                                                   : std::filesystem::path{},
    };
    Hockey::Result<Hockey::RuntimeConfigLoadResult> runtimeConfig = Hockey::LoadRuntimeConfig(configInfo);
    if (!runtimeConfig) {
        HK_CLIENT_INFO("Client config load failed: {}", runtimeConfig.error);
        return false;
    }
    m_Config = runtimeConfig.value.config;
    m_UserConfigPath = runtimeConfig.value.userConfigPath;
    m_AudioSettings = Hockey::LoadAudioSettings(m_Config);
    m_UISettings = Hockey::LoadUISettings(m_Config);
    m_AnimationSettings.enabled = m_Config.GetBool("animation.enabled", m_AnimationSettings.enabled);
    m_AnimationSettings.enableAnimationEvents =
        m_Config.GetBool("animation.enable_animation_events", m_AnimationSettings.enableAnimationEvents);
    m_AnimationSettings.enableRootMotion =
        m_Config.GetBool("animation.enable_root_motion", m_AnimationSettings.enableRootMotion);
    m_AnimationSettings.defaultBlendTimeSeconds = static_cast<float>(
        m_Config.GetDouble("animation.default_blend_time_seconds", m_AnimationSettings.defaultBlendTimeSeconds));
    m_AnimationSettings.maxBones =
        static_cast<std::uint32_t>(std::max(0, m_Config.GetInt("animation.max_bones",
                                                               static_cast<int>(m_AnimationSettings.maxBones))));
    m_AnimationSettings.debugDrawSkeletons =
        m_Config.GetBool("animation.debug_draw_skeletons", m_AnimationSettings.debugDrawSkeletons);
    m_AnimationSettings.logAnimationEvents =
        m_Config.GetBool("animation.log_animation_events", m_AnimationSettings.logAnimationEvents);
    m_AnimationSystem.SetSettings(m_AnimationSettings);
    m_UIEnabled = m_UISettings.enabled && !cmd.Has("--no-ui");
    m_PresentationSettings.enabled = m_Config.GetBool("presentation.interpolate_gameplay", true);
    m_PresentationSettings.interpolatePlayers = m_Config.GetBool("presentation.interpolate_players", true);
    m_PresentationSettings.interpolatePuck = m_Config.GetBool("presentation.interpolate_puck", true);
    m_MovementSmoothnessTraceSettings.enabled =
        m_Config.GetBool("diagnostics.movement_smoothness_trace", false) || cmd.Has("--movement-smoothness-trace");
    m_MovementSmoothnessTraceSettings.playerIndex =
        static_cast<uint32_t>(std::max(0, cmd.GetInt("--movement-smoothness-trace-player",
                                                     m_Config.GetInt("diagnostics.movement_smoothness_trace_player_index",
                                                                     0))));
    m_FollowCameraSettings.enabled = m_Config.GetBool("camera.follow_local_player", true);
    m_FollowCameraSettings.playerIndex =
        static_cast<uint32_t>(std::max(0, m_Config.GetInt("camera.follow_player_index", 0)));
    m_FollowCameraSettings.offset.x = static_cast<float>(m_Config.GetDouble("camera.follow_offset_x", 0.0));
    m_FollowCameraSettings.offset.y = static_cast<float>(m_Config.GetDouble("camera.follow_offset_y", 7.5));
    m_FollowCameraSettings.offset.z = static_cast<float>(m_Config.GetDouble("camera.follow_offset_z", 10.0));
    m_FollowCameraSettings.lookAtOffset.x =
        static_cast<float>(m_Config.GetDouble("camera.follow_look_at_offset_x", 0.0));
    m_FollowCameraSettings.lookAtOffset.y =
        static_cast<float>(m_Config.GetDouble("camera.follow_look_at_offset_y", 1.2));
    m_FollowCameraSettings.lookAtOffset.z =
        static_cast<float>(m_Config.GetDouble("camera.follow_look_at_offset_z", 0.0));
    m_FollowCameraSettings.positionResponse =
        static_cast<float>(m_Config.GetDouble("camera.follow_position_response", 9.0));
    m_FollowCameraSettings.lookAtResponse =
        static_cast<float>(m_Config.GetDouble("camera.follow_look_at_response", 12.0));
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
    Hockey::RegisterAnimationComponentSerialization();
    Hockey::RegisterAudioComponents();
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
            m_PresentationState.Reset();
            MarkMovementSmoothnessPresentationReset();
            m_PresentationState.CaptureFixedStep(m_Scene);
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

    m_AudioCueMap = Hockey::ResolveHockeyAudioCueMap(m_Config, m_AssetsReady ? &m_AssetManager : nullptr);
    m_AudioController.SetCueMap(m_AudioCueMap);

    if (m_AudioSettings.enabled && m_AssetsReady) {
        m_AudioSystem = std::make_unique<Hockey::AudioSystem>(m_AudioSettings);
        if (const Hockey::Status status = m_AudioSystem->Initialize(&m_AssetManager); status) {
            m_AudioController.SetSink(m_AudioSystem.get());
            m_AudioReady = true;
            HK_CLIENT_INFO("Audio initialized");
        } else {
            HK_CLIENT_INFO("Audio disabled: {}", status.error);
            m_AudioSystem.reset();
        }
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

Hockey::Status GameClientApp::SaveUserSettings() {
    return Hockey::SaveRuntimeUserConfig(m_Config, m_UserConfigPath);
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
    if (m_AudioReady && action != Hockey::UIAction::Quit) {
        const Hockey::AssetID clip = action == Hockey::UIAction::ReturnToMenu ? m_AudioCueMap.uiBack
                                    : action == Hockey::UIAction::StartOffline ? m_AudioCueMap.uiConfirm
                                                                               : m_AudioCueMap.uiClick;
        m_AudioSystem->PlayOneShot(clip, Hockey::AudioBusType::UI, 1.0f);
    }

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
    m_LocalInputAccumulator.Reset();
    m_ResetMatchRequested = false;
    m_PresentationState.Reset();
    MarkMovementSmoothnessPresentationReset();
    m_PresentationState.CaptureFixedStep(m_Scene);
    m_FollowCameraState = {};
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
    if (m_AudioSystem != nullptr) {
        m_AudioController.SetSink(nullptr);
        m_AudioSystem->Shutdown();
        m_AudioSystem.reset();
        m_AudioReady = false;
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

Hockey::GameplayInputFrame GameClientApp::BuildLocalInputSample() {
    Hockey::GameplayInputFrame input;
    input.playerIndex = 0;
    input.inputSequence = ++m_LocalInputSequence;
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

void GameClientApp::AccumulateLocalInputSample() {
    if (!m_LocalGameplayEnabled || !m_GameplayWorld.IsInitialized() ||
        (m_UIEnabled && m_ClientFlow.ActiveScreen() != Hockey::UIScreenId::MatchHud)) {
        m_LocalInputAccumulator.Reset();
        m_ResetMatchRequested = false;
        return;
    }

    m_ResetMatchRequested = m_ResetMatchRequested || Hockey::Input::WasKeyPressed(Hockey::KeyCode::R);
    m_LocalInputAccumulator.Accumulate(BuildLocalInputSample());
}

void GameClientApp::MarkMovementSmoothnessPresentationReset() {
    if (m_MovementSmoothnessTraceSettings.enabled) {
        m_MovementSmoothnessPresentationResetThisFrame = true;
    }
}

void GameClientApp::StepSimulation(float deltaTime) {
    m_MovementSmoothnessFixedStepsThisFrame = 0;
    if (!m_PhysicsReady && !m_LocalGameplayEnabled) {
        return;
    }
    const int steps = m_SimulationTimestep.Accumulate(static_cast<double>(deltaTime));
    m_MovementSmoothnessFixedStepsThisFrame = steps;
    const float fixedDelta = static_cast<float>(m_SimulationTimestep.GetFixedDeltaSeconds());
    for (int i = 0; i < steps; ++i) {
        const uint64_t tick = m_SimulationTimestep.GetTick() + 1;
        std::vector<Hockey::GameplayEvent> gameplayEvents;
        MovementTraceEntityState beforeGameplay;
        if (m_MovementSmoothnessTraceSettings.enabled) {
            beforeGameplay = CaptureMovementTraceEntityState(
                m_Scene, m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr,
                m_MovementSmoothnessTraceSettings.playerIndex);
        }

        if (m_LocalGameplayEnabled && m_GameplayWorld.IsInitialized()) {
            if (m_ResetMatchRequested) {
                m_GameplayWorld.ResetMatch(m_Scene);
                m_PresentationState.Reset();
                MarkMovementSmoothnessPresentationReset();
                m_PresentationState.CaptureFixedStep(m_Scene);
                m_LocalInputAccumulator.Reset();
                m_ResetMatchRequested = false;
            }
            m_GameplayWorld.PushInput(m_LocalInputAccumulator.Consume(tick));
            m_GameplayWorld.FixedUpdate(m_Scene, fixedDelta, tick);
            gameplayEvents = m_GameplayWorld.DrainEvents();
            if (m_GameplaySettings.logGameplayEvents) {
                for (const Hockey::GameplayEvent& event : gameplayEvents) {
                    HK_CLIENT_INFO("Gameplay event: {}", Hockey::GameplayEventTypeToString(event.type));
                }
            }
            if (m_AudioReady) {
                for (const Hockey::GameplayEvent& event : gameplayEvents) {
                    HandleAudioGameplayEvent(event);
                }
            }
        }
        MovementTraceEntityState afterGameplay;
        if (m_MovementSmoothnessTraceSettings.enabled) {
            afterGameplay = CaptureMovementTraceEntityState(
                m_Scene, m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr,
                m_MovementSmoothnessTraceSettings.playerIndex);
        }

        if (m_PhysicsReady) {
            m_Scene.OnFixedUpdate(fixedDelta);
        }
        MovementTraceEntityState afterPhysicsStep;
        if (m_MovementSmoothnessTraceSettings.enabled) {
            afterPhysicsStep = CaptureMovementTraceEntityState(
                m_Scene, m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr,
                m_MovementSmoothnessTraceSettings.playerIndex);
        }
        if (m_LocalGameplayEnabled && m_GameplayWorld.IsInitialized()) {
            m_GameplayWorld.SyncPhysicsState(m_Scene);
            const Hockey::GameplaySnapshot snapshot =
                Hockey::BuildGameplaySnapshot(m_Scene, tick, m_GameplayWorld.GetTuning());
            m_AnimationController.Apply(m_Scene, BuildHockeyAnimationFrame(snapshot, gameplayEvents));
            if (m_AssetsReady) {
                const Hockey::Status status = m_AnimationSystem.Update(m_Scene, m_AssetManager, fixedDelta);
                if (!status) {
                    HK_CLIENT_INFO("Animation update failed: {}", status.error);
                }
            }
            m_PresentationState.CaptureFixedStep(m_Scene);
        }
        if (m_MovementSmoothnessTraceSettings.enabled) {
            const MovementTraceEntityState afterPhysicsSync = CaptureMovementTraceEntityState(
                m_Scene, m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr,
                m_MovementSmoothnessTraceSettings.playerIndex);
            HockeyClient::GameplayPresentationState::Sample presentationSample;
            float presentationSampleDelta = 0.0f;
            if (afterPhysicsSync.valid) {
                GetPresentationSampleTrace(
                    m_PresentationState, afterPhysicsSync.entityId, presentationSample, presentationSampleDelta);
            }
            LogMovementSmoothnessTick(tick,
                                      m_MovementSmoothnessTraceSettings.playerIndex,
                                      m_MovementSmoothnessPresentationResetThisFrame,
                                      beforeGameplay,
                                      afterGameplay,
                                      afterPhysicsStep,
                                      afterPhysicsSync,
                                      presentationSampleDelta);
        }
        m_SimulationTimestep.AdvanceTick();
    }
    if (m_PhysicsReady) {
        const std::vector<Hockey::PhysicsContactEvent> contacts = m_PhysicsSystem->World().DrainContactEvents();
        if (m_AudioReady) {
            for (const Hockey::PhysicsContactEvent& contact : contacts) {
                HandleAudioPhysicsContact(contact);
            }
        }
        m_PhysicsSystem->World().DrainTriggerEvents();
    }
}

void GameClientApp::HandleAudioGameplayEvent(const Hockey::GameplayEvent& event) {
    switch (event.type) {
    case Hockey::GameplayEventType::CountdownBeep:
    case Hockey::GameplayEventType::FaceoffStarted:
        m_AudioController.Trigger(Hockey::AudioCueId::Countdown, event.position);
        break;
    case Hockey::GameplayEventType::PuckShot:
        m_AudioController.Trigger(Hockey::AudioCueId::Shot, event.position);
        break;
    case Hockey::GameplayEventType::GoalieShieldStarted:
        m_AudioController.Trigger(Hockey::AudioCueId::GoalieShield, event.position);
        break;
    case Hockey::GameplayEventType::PlayerBoosted:
        m_AudioController.Trigger(Hockey::AudioCueId::PlayerBoost, event.position, 0.8f);
        break;
    default:
        break;
    }
}

void GameClientApp::HandleAudioPhysicsContact(const Hockey::PhysicsContactEvent& contact) {
    if (contact.type != Hockey::PhysicsContactEvent::Type::Started) {
        return;
    }

    auto contactUsesMetalCue = [this](Hockey::UUID id) {
        Hockey::Entity entity = m_Scene.FindEntityByUUID(id);
        if (!entity) {
            return false;
        }
        std::string material;
        if (entity.HasComponent<Hockey::PhysicsMaterialComponent>()) {
            material = entity.GetComponent<Hockey::PhysicsMaterialComponent>().materialName;
        } else if (entity.HasComponent<Hockey::RigidBodyComponent>()) {
            material = entity.GetComponent<Hockey::RigidBodyComponent>().materialName;
        }
        return material.find("Metal") != std::string::npos || material.find("Board") != std::string::npos ||
               material.find("Glass") != std::string::npos || material.find("Goal") != std::string::npos;
    };

    const Hockey::AudioCueId cue =
        (contactUsesMetalCue(contact.entityA) || contactUsesMetalCue(contact.entityB))
            ? Hockey::AudioCueId::PuckMetalCollision
            : Hockey::AudioCueId::PuckCollision;
    const float volume = contact.impulse > 0.0f ? std::clamp(contact.impulse / 8.0f, 0.2f, 1.0f) : 0.35f;
    m_AudioController.Trigger(cue, contact.contactPoint, volume);
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
    AccumulateLocalInputSample();
    StepSimulation(deltaTime);
    if (m_AudioReady) {
        m_AudioSystem->OnUpdate(m_Scene, deltaTime);
    }
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

    {
        HockeyClient::ScopedGameplayPresentationTransforms presentationScope(
            m_Scene,
            m_PresentationState,
            static_cast<float>(m_SimulationTimestep.GetInterpolationAlpha()),
            m_PresentationSettings);

        if (m_LocalGameplayEnabled && m_GameplayWorld.IsInitialized()) {
            HockeyClient::UpdateGameplayFollowCamera(m_Scene, deltaTime, m_FollowCameraSettings, m_FollowCameraState);
        }

        if (m_MovementSmoothnessTraceSettings.enabled) {
            const MovementTraceEntityState presentedState = CaptureMovementTraceEntityState(
                m_Scene, m_PhysicsSystem != nullptr ? &m_PhysicsSystem->World() : nullptr,
                m_MovementSmoothnessTraceSettings.playerIndex);
            HockeyClient::GameplayPresentationState::Sample presentationSample;
            float presentationSampleDelta = 0.0f;
            const bool hasPresentationSample =
                presentedState.valid && GetPresentationSampleTrace(m_PresentationState,
                                                                    presentedState.entityId,
                                                                    presentationSample,
                                                                    presentationSampleDelta);
            glm::vec3 cameraPosition{0.0f};
            const bool hasCameraPosition = FindActiveCameraPosition(m_Scene, cameraPosition);
            LogMovementSmoothnessFrame(m_MovementSmoothnessTraceFrameIndex,
                                       deltaTime,
                                       m_MovementSmoothnessFixedStepsThisFrame,
                                       m_SimulationTimestep.GetTick(),
                                       static_cast<float>(m_SimulationTimestep.GetInterpolationAlpha()),
                                       m_MovementSmoothnessTraceSettings.playerIndex,
                                       m_MovementSmoothnessPresentationResetThisFrame,
                                       presentedState,
                                       presentationSample,
                                       hasPresentationSample,
                                       presentationSampleDelta,
                                       cameraPosition,
                                       hasCameraPosition);
            ++m_MovementSmoothnessTraceFrameIndex;
            m_MovementSmoothnessPresentationResetThisFrame = false;
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
