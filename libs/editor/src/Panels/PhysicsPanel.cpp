#include "Hockey/Editor/Panels/PhysicsPanel.hpp"

#include <imgui.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"

namespace Hockey {

PhysicsPanel::PhysicsPanel() : Panel(EditorPanelNames::kPhysics) {}

void PhysicsPanel::OnImGui(EditorContext& context) {
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    EditorPhysicsPreview* preview = context.physicsPreview;
    EditorGameplayPreview* gameplayPreview = context.gameplayPreview;
    Scene* scene = context.activeScene;

    if (preview == nullptr || gameplayPreview == nullptr || scene == nullptr) {
        ImGui::TextUnformatted("Physics preview unavailable.");
        EndWindow();
        return;
    }

    PhysicsViewState& view = context.physicsView;

    ImGui::SeparatorText("Preview");

    const bool active = preview->IsActive();
    bool enabled = view.previewEnabled;
    if (ImGui::Checkbox("Preview Physics", &enabled)) {
        view.previewEnabled = enabled;
        if (enabled) {
            preview->Start(*scene);
        } else {
            preview->Stop(*scene);
        }
        view.previewRunning = preview->IsRunning();
    }

    ImGui::BeginDisabled(!active);

    const bool running = preview->IsRunning();
    if (running) {
        if (ImGui::Button("Pause")) {
            preview->Pause();
        }
    } else {
        if (ImGui::Button("Play")) {
            preview->Play();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        preview->StepOnce(*scene);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        preview->Reset(*scene);
    }
    view.previewRunning = preview->IsRunning();

    ImGui::EndDisabled();

    if (!active) {
        ImGui::TextDisabled("Enable preview to simulate the scene.");
        ImGui::TextDisabled("Stopping/Reset restores the original transforms.");
    }

    ImGui::SeparatorText("Gameplay Preview");

    GameplayPreviewState& gameplay = context.gameplayView;
    bool gameplayEnabled = gameplay.previewEnabled;
    if (ImGui::Checkbox("Preview Gameplay", &gameplayEnabled)) {
        gameplay.previewEnabled = gameplayEnabled;
        if (gameplayEnabled) {
            if (const Status status = gameplayPreview->Start(*scene, *preview); !status) {
                HK_EDITOR_ERROR("Gameplay preview failed: {}", status.error);
                gameplay.previewEnabled = false;
            }
        } else {
            gameplayPreview->Stop(*scene, *preview);
        }
        gameplay.previewRunning = gameplayPreview->IsRunning();
        view.previewEnabled = preview->IsActive();
        view.previewRunning = preview->IsRunning();
    }

    ImGui::BeginDisabled(gameplayPreview->IsActive());
    if (ImGui::Button("Initialize Match")) {
        if (const Status status = gameplayPreview->Start(*scene, *preview); !status) {
            HK_EDITOR_ERROR("Initialize gameplay preview failed: {}", status.error);
            gameplay.previewEnabled = false;
        } else {
            gameplay.previewEnabled = true;
        }
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!gameplayPreview->IsActive());
    ImGui::SameLine();
    if (gameplayPreview->IsRunning()) {
        if (ImGui::Button("Pause Gameplay Preview")) {
            gameplayPreview->Pause();
        }
    } else {
        if (ImGui::Button("Start Gameplay Preview")) {
            gameplayPreview->Play();
        }
    }
    if (ImGui::Button("Step Gameplay Tick")) {
        gameplayPreview->StepOnce(*scene, *preview);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Match")) {
        gameplayPreview->Reset(*scene, *preview);
    }
    ImGui::EndDisabled();

    bool showDebug = gameplay.showDebug;
    if (ImGui::Checkbox("Show Gameplay Debug", &showDebug)) {
        gameplay.showDebug = showDebug;
        gameplayPreview->SetDebugDrawEnabled(showDebug);
    }
    gameplay.previewRunning = gameplayPreview->IsRunning();

    ImGui::SeparatorText("Visualization");
    ImGui::Checkbox("Show Colliders", &view.showColliders);
    ImGui::Checkbox("Show Triggers", &view.showTriggers);
    ImGui::Checkbox("Show Body Centers", &view.showBodyCenters);
    ImGui::Checkbox("Show Contacts", &view.showContacts);

    EndWindow();
}

} // namespace Hockey
