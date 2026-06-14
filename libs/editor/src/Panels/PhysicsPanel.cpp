#include "Hockey/Editor/Panels/PhysicsPanel.hpp"

#include <imgui.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"

namespace Hockey {

PhysicsPanel::PhysicsPanel() : Panel(EditorPanelNames::kPhysics) {}

void PhysicsPanel::OnImGui(EditorContext& context) {
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    EditorPhysicsPreview* preview = context.physicsPreview;
    Scene* scene = context.activeScene;

    if (preview == nullptr || scene == nullptr) {
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

    ImGui::SeparatorText("Visualization");
    ImGui::Checkbox("Show Colliders", &view.showColliders);
    ImGui::Checkbox("Show Triggers", &view.showTriggers);
    ImGui::Checkbox("Show Body Centers", &view.showBodyCenters);
    ImGui::Checkbox("Show Contacts", &view.showContacts);

    EndWindow();
}

} // namespace Hockey
