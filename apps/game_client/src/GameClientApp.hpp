#pragma once
#include "GameplayCamera.hpp"
#include "GameplayPresentation.hpp"

#include "Hockey/Animation/AnimationSettings.hpp"
#include "Hockey/Animation/AnimationSystem.hpp"
#include "Hockey/Animation/HockeyAnimationController.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Audio/AudioSettings.hpp"
#include "Hockey/Audio/AudioSystem.hpp"
#include "Hockey/Audio/HockeyAudioController.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FixedTimestep.hpp"
#include "Hockey/Core/WindowedApplication.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/ClientFlowRunner.hpp"
#include "Hockey/UI/RmlUiContext.hpp"
#include "Hockey/UI/UIInputMapper.hpp"
#include "Hockey/UI/UISettings.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace Hockey {
class PhysicsSystem;
}

class GameClientApp final : public Hockey::WindowedApplication {
public:
    using WindowedApplication::WindowedApplication;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;
    void OnEvent(const Hockey::Event& event) override;

private:
    struct MovementSmoothnessTraceSettings {
        bool enabled = false;
        uint32_t playerIndex = 0;
    };

    bool LoadRuntimeUIScreen();
    void BindRuntimeUIActions();
    void QueueRuntimeUIAction(Hockey::UIAction action);
    bool LoadOfflineGameplayScene(const std::string& scenePath);
    Hockey::Status SaveUserSettings();

    void MarkMovementSmoothnessPresentationReset();
    void AccumulateLocalInputSample();
    void StepSimulation(float deltaTime);
    Hockey::GameplayInputFrame BuildLocalInputSample();
    void HandleAudioGameplayEvent(const Hockey::GameplayEvent& event);
    void HandleAudioPhysicsContact(const Hockey::PhysicsContactEvent& contact);
    void SubmitPhysicsDebugDraw();
    void SubmitGameplayDebugDraw();

    Hockey::Config m_Config;
    std::filesystem::path m_UserConfigPath;
    Hockey::Scene m_Scene{"Game Scene"};
    Hockey::GameplayWorld m_GameplayWorld;
    Hockey::GameplaySettings m_GameplaySettings;
    Hockey::GameplayTuning m_GameplayTuning;
    HockeyClient::GameplayFollowCameraSettings m_FollowCameraSettings;
    HockeyClient::GameplayFollowCameraState m_FollowCameraState;
    HockeyClient::GameplayPresentationSettings m_PresentationSettings;
    HockeyClient::GameplayPresentationState m_PresentationState;
    MovementSmoothnessTraceSettings m_MovementSmoothnessTraceSettings;
    Hockey::Renderer m_Renderer;
    Hockey::AssetManager m_AssetManager;
    Hockey::AnimationSettings m_AnimationSettings;
    Hockey::AnimationSystem m_AnimationSystem;
    Hockey::HockeyAnimationController m_AnimationController;
    Hockey::AudioSettings m_AudioSettings;
    std::unique_ptr<Hockey::AudioSystem> m_AudioSystem;
    Hockey::HockeyAudioController m_AudioController;
    Hockey::AudioCueMap m_AudioCueMap;
    Hockey::UISettings m_UISettings;
    Hockey::ClientFlowRunner m_ClientFlow;
    Hockey::RmlUiContext m_UIContext;
    Hockey::UIInputMapper m_UIInput;
    bool m_RendererReady = false;
    bool m_AssetsReady = false;
    bool m_UIEnabled = false;
    bool m_UIReloadRequested = false;
    bool m_AudioReady = false;

    Hockey::PhysicsSystem* m_PhysicsSystem = nullptr; // owned by m_Scene
    Hockey::FixedTimestep m_SimulationTimestep{60.0};
    Hockey::GameplayInputAccumulator m_LocalInputAccumulator;
    uint64_t m_LocalInputSequence = 0;
    bool m_LocalGameplayEnabled = false;
    bool m_PhysicsReady = false;
    bool m_PhysicsDebug = false;
    bool m_ResetMatchRequested = false;
    uint64_t m_MovementSmoothnessTraceFrameIndex = 0;
    int m_MovementSmoothnessFixedStepsThisFrame = 0;
    bool m_MovementSmoothnessPresentationResetThisFrame = false;
    bool m_AutoScreenshotPending = false;
    std::string m_ScreenshotPrefix = "game";

    uint32_t m_LastWidth = 0;
    uint32_t m_LastHeight = 0;
};
