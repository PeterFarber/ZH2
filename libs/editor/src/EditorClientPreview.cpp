#include "Hockey/Editor/EditorClientPreview.hpp"

#include <algorithm>
#include <string>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Gameplay/GameplayTypes.hpp"
#include "Hockey/Gameplay/Simulation/GameplaySnapshot.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/UI/ClientFlowSerializer.hpp"
#include "Hockey/UI/RmlUiRenderInterface.hpp"
#include "Hockey/UI/UISettings.hpp"
#include "Hockey/UI/ViewModels.hpp"

namespace Hockey {
namespace {

std::string PreviewPhaseLabel(MatchPhase phase) {
    switch (phase) {
    case MatchPhase::NotStarted: return "Not started";
    case MatchPhase::Warmup: return "Warmup";
    case MatchPhase::FaceoffSetup: return "Faceoff setup";
    case MatchPhase::Faceoff: return "Faceoff";
    case MatchPhase::Playing: return "Playing";
    case MatchPhase::GoalScored: return "Goal";
    case MatchPhase::PeriodEnded: return "Period ended";
    case MatchPhase::MatchEnded: return "Final";
    case MatchPhase::Paused: return "Paused";
    }
    return "Not started";
}

HudViewModel BuildPreviewHudViewModel(const GameplaySnapshot& snapshot, std::uint32_t localPlayerIndex) {
    HudViewModel hud;
    hud.period = static_cast<int>(snapshot.match.period);
    hud.homeScore = static_cast<int>(snapshot.match.homeScore);
    hud.awayScore = static_cast<int>(snapshot.match.awayScore);
    hud.clockText = FormatClockText(snapshot.match.periodTimeRemaining);
    hud.phaseLabel = PreviewPhaseLabel(snapshot.match.phase);
    hud.possessionLabel = snapshot.puck.possessingPlayer.IsValid() ? "Possessed" : "Loose puck";

    for (const PlayerGameplaySnapshot& player : snapshot.players) {
        const std::string label = std::string(GameplayTeamToString(player.team)) + " " +
                                  GameplayRoleToString(player.role) + " " +
                                  std::to_string(player.playerIndex + 1);
        if (player.playerIndex == localPlayerIndex) {
            hud.localPlayerLabel = label;
            hud.shotChargeRatio = player.shotChargeRatio;
        }
        if (snapshot.puck.possessingPlayer == player.entity || player.hasPuck) {
            hud.possessionLabel = label;
        }
    }

    if (hud.localPlayerLabel.empty()) {
        hud.localPlayerLabel = "Player " + std::to_string(localPlayerIndex + 1);
    }
    return hud;
}

void ApplyHudViewModel(RmlUiContext& ui, const HudViewModel& hud) {
    ui.SetElementText("home-score", std::to_string(hud.homeScore));
    ui.SetElementText("away-score", std::to_string(hud.awayScore));
    ui.SetElementText("match-clock", hud.clockText);
    ui.SetElementText("period-label", "P" + std::to_string(hud.period));
    ui.SetElementText("phase-label", hud.phaseLabel);
    ui.SetElementText("local-player-label", hud.localPlayerLabel);
    ui.SetElementText("possession-label", hud.possessionLabel);
    ui.SetElementText("shot-charge", FormatShotCharge(hud.shotChargeRatio));
    ui.SetElementText("hud-notification", hud.notificationText);
}

} // namespace

Status EditorClientPreview::Start(EditorContext& context, const std::filesystem::path& flowPath) {
    if (m_Active) {
        Stop(context);
    }
    if (context.renderer == nullptr || context.activeScene == nullptr) {
        return Status::Fail("Client Preview requires an active scene and renderer");
    }

    UISettings settings = MakeDefaultUISettings();
    if (context.config != nullptr) {
        settings = LoadUISettings(*context.config);
    }

    m_FlowPath = flowPath.empty() ? Paths::Resolve(settings.startFlow) : Paths::Resolve(flowPath);
    ClientFlow flow = MakeDefaultClientFlow();
    if (const Result<ClientFlow> loaded = ClientFlowSerializer::Load(m_FlowPath)) {
        flow = loaded.value;
    }
    m_ClientFlow.SetFlow(flow);
    m_ClientFlow.Boot();

    RmlUiContextDesc desc;
    desc.renderer = context.renderer;
    desc.root = Paths::Get().root;
    desc.settings = settings;
    desc.name = "editor-client-preview-ui";
    desc.width = std::max(1u, context.clientPreview.viewportWidth);
    desc.height = std::max(1u, context.clientPreview.viewportHeight);
    if (!m_UIContext.Initialize(desc)) {
        return Status::Fail("RmlUi initialization failed for Client Preview");
    }
    if (!LoadActiveScreen()) {
        m_UIContext.Shutdown();
        return Status::Fail("Client Preview failed to load startup UI screen");
    }

    m_RestoreDirty = context.sceneDirty;
    if (context.gameplayPreview != nullptr && context.physicsPreview != nullptr) {
        if (const Status status = context.gameplayPreview->Start(*context.activeScene, *context.physicsPreview);
            status) {
            context.gameplayPreview->SetInputEnabled(false);
            context.gameplayPreview->Play();
            m_StartedGameplayPreview = true;
        }
    }

    m_Active = true;
    context.clientPreview.previewEnabled = true;
    context.clientPreview.previewRunning = false;
    return Status::Ok();
}

void EditorClientPreview::Stop(EditorContext& context) {
    if (!m_Active && !m_UIContext.IsInitialized()) {
        return;
    }

    if (m_StartedGameplayPreview && context.activeScene != nullptr && context.gameplayPreview != nullptr &&
        context.physicsPreview != nullptr) {
        context.gameplayPreview->Stop(*context.activeScene, *context.physicsPreview);
        m_StartedGameplayPreview = false;
    }
    m_UIContext.Shutdown();
    m_Active = false;
    m_ReloadRequested = false;
    context.clientPreview.previewEnabled = false;
    context.clientPreview.previewRunning = false;
    context.clientPreview.gameInputActive = false;
    context.sceneDirty = m_RestoreDirty;
}

void EditorClientPreview::Update(EditorContext& context, float /*deltaTime*/) {
    if (!m_Active) {
        return;
    }
    if (m_ReloadRequested) {
        LoadActiveScreen();
        m_ReloadRequested = false;
    }
    ApplyHud(context);
    m_UIContext.Update();
    context.clientPreview.previewRunning = IsRunning();
}

void EditorClientPreview::RenderToTarget(EditorContext& context,
                                         TextureHandle target,
                                         std::uint32_t width,
                                         std::uint32_t height) {
    if (!m_Active || context.renderer == nullptr || !target.IsValid() || width == 0 || height == 0) {
        return;
    }
    m_UIContext.SetDimensions(width, height);

    bool renderedScene = false;
    if (context.activeScene != nullptr && m_ClientFlow.ActiveScreen() == UIScreenId::MatchHud) {
        CameraRenderData camera;
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        if (FindActiveCamera(*context.activeScene, aspect, camera)) {
            context.renderer->RenderSceneToTarget(*context.activeScene, camera, target);
            renderedScene = true;
        }
    }

    m_UIContext.Render();
    const RmlUiRenderInterface* renderInterface = m_UIContext.RenderInterface();
    if (renderInterface != nullptr) {
        context.renderer->RenderUIOverlayToTarget(target, renderInterface->DrawCommands(), !renderedScene);
        if (RmlUiRenderInterface* mutableRenderInterface = m_UIContext.RenderInterface()) {
            mutableRenderInterface->ClearDrawCommands();
        }
    }
}

void EditorClientPreview::HandlePointerInput(int x, int y, bool leftPressed, bool leftReleased, float wheelY) {
    if (!m_Active || !m_UIContext.IsInitialized()) {
        return;
    }
    m_UIContext.ProcessMouseMove(x, y);
    if (leftPressed) {
        m_UIContext.ProcessMouseButton(0, true);
    }
    if (leftReleased) {
        m_UIContext.ProcessMouseButton(0, false);
    }
    if (wheelY != 0.0f) {
        m_UIContext.ProcessMouseWheel(0.0f, wheelY);
    }
}

bool EditorClientPreview::LoadActiveScreen() {
    if (!m_UIContext.IsInitialized()) {
        return false;
    }
    m_UIContext.UnloadAllDocuments();
    m_UIContext.Update();
    if (!m_UIContext.LoadDocument(m_ClientFlow.Flow().ScreenDocument(m_ClientFlow.ActiveScreen()))) {
        return false;
    }
    BindRuntimeActions();
    return true;
}

void EditorClientPreview::BindRuntimeActions() {
    m_UIContext.BindClickAction("play-offline", [this]() { QueueAction(UIAction::StartOffline); });
    m_UIContext.BindClickAction("open-settings", [this]() { QueueAction(UIAction::OpenSettings); });
    m_UIContext.BindClickAction("open-lobby", [this]() { QueueAction(UIAction::OpenLobby); });
    m_UIContext.BindClickAction("quit-game", [this]() { QueueAction(UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("settings-back", [this]() { QueueAction(UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("lobby-back", [this]() { QueueAction(UIAction::ReturnToMenu); });
    m_UIContext.BindClickAction("resume-game", [this]() { QueueAction(UIAction::Resume); });
    m_UIContext.BindClickAction("return-to-menu", [this]() { QueueAction(UIAction::ReturnToMenu); });
}

void EditorClientPreview::QueueAction(UIAction action) {
    m_ClientFlow.Dispatch(action);
    if (action == UIAction::StartOffline) {
        (void)m_ClientFlow.TakeRequestedGameplayScene();
        m_ClientFlow.Dispatch(UIAction::Resume);
    }
    m_ReloadRequested = true;
}

void EditorClientPreview::ApplyHud(EditorContext& context) {
    if (!m_Active || m_ClientFlow.ActiveScreen() != UIScreenId::MatchHud || context.activeScene == nullptr) {
        return;
    }
    const GameplaySnapshot snapshot = BuildGameplaySnapshot(*context.activeScene, 0);
    ApplyHudViewModel(m_UIContext, BuildPreviewHudViewModel(snapshot, 0));
}

} // namespace Hockey
