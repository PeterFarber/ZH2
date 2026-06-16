#include "Hockey/Editor/Panels/PreferencesPanel.hpp"

#include <imgui.h>

#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorSettings.hpp"

namespace Hockey {

PreferencesPanel::PreferencesPanel() : Panel(EditorPanelNames::kPreferences, /*openByDefault=*/false) {}

void PreferencesPanel::OnImGui(EditorContext& context) {
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    EditorSettings& s = context.settings;
    bool changed = false;

    if (ImGui::CollapsingHeader("Autosave", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Autosave enabled", &s.autosaveEnabled);
        ImGui::BeginDisabled(!s.autosaveEnabled);
        changed |= ImGui::DragInt("Interval (seconds)", &s.autosaveIntervalSeconds, 1.0f, 5, 3600);
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Validate before save", &s.validateBeforeSave);
        changed |= ImGui::Checkbox("Validate after load", &s.validateAfterLoad);
    }

    if (ImGui::CollapsingHeader("Grid & Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Show grid", &s.showGrid);
        changed |= ImGui::DragFloat("Grid spacing", &s.gridSpacing, 0.05f, 0.05f, 100.0f, "%.2f");
        ImGui::Separator();
        changed |= ImGui::Checkbox("Snap enabled", &s.snapEnabled);
        changed |= ImGui::DragFloat("Move snap", &s.moveSnap, 0.01f, 0.001f, 100.0f, "%.3f");
        changed |= ImGui::DragFloat("Rotate snap (deg)", &s.rotateSnapDegrees, 0.5f, 1.0f, 180.0f, "%.1f");
        changed |= ImGui::DragFloat("Scale snap", &s.scaleSnap, 0.01f, 0.001f, 100.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat("Move speed", &s.cameraMoveSpeed, 0.1f, 0.1f, 1000.0f, "%.2f");
        changed |= ImGui::DragFloat("Fast multiplier", &s.cameraFastMultiplier, 0.1f, 1.0f, 50.0f, "%.2f");
        changed |= ImGui::DragFloat("Mouse sensitivity", &s.cameraMouseSensitivity, 0.005f, 0.01f, 2.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("Startup", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Restore last scene on launch", &s.restoreLastScene);
    }

    if (ImGui::CollapsingHeader("Asset Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Auto-discover raw assets", &s.assetsAutoDiscover);
        changed |= ImGui::Checkbox("Auto-import on change", &s.assetsAutoImport);
        changed |= ImGui::Checkbox("Auto-cook dirty assets", &s.assetsAutoCookDirty);
        changed |= ImGui::Checkbox("Hot reload", &s.assetsHotReload);
    }

    if (changed) {
        s.Save(EditorSettings::DefaultPath());
    }

    EndWindow();
}

} // namespace Hockey
