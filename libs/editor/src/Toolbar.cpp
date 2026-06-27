#include "Hockey/Editor/Toolbar.hpp"

#include <cstdio>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {

namespace {

void VerticalSeparator() {
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
}

float EditorScaleControlWidth() {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float buttonWidth = ImGui::GetFrameHeight();
    const float percentWidth = ImGui::CalcTextSize("200%").x;
    return buttonWidth + style.ItemSpacing.x + percentWidth;
}

void DrawEditorScaleControl(EditorContext& ctx, EditorApp& app) {
    char percent[16];
    std::snprintf(percent, sizeof(percent), "%.0f%%", ctx.settings.editorScale * 100.0f);

    if (EditorIconButton(EditorIcon::Settings, "Editor Scale", "Adjusts the size of editor text and controls")) {
        ImGui::OpenPopup("Editor Scale");
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(percent);
    EditorTooltip::ForLastItem("Current editor scale");

    if (ImGui::BeginPopup("Editor Scale")) {
        ImGui::TextUnformatted("Editor Scale");
        ImGui::Separator();

        float scale = ctx.settings.editorScale;
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("##EditorScaleSlider",
                               &scale,
                               EditorSettings::kMinEditorScale,
                               EditorSettings::kMaxEditorScale,
                               "%.2fx")) {
            app.SetEditorScale(scale);
        }
        EditorTooltip::ForLastItem(
            "Adjusts the size of editor text and controls without changing scene render scale.");

        if (ImGui::Button("Reset to 100%")) {
            app.SetEditorScale(1.0f);
        }
        EditorTooltip::ForLastItem("Restores the default editor scale.");

        ImGui::EndPopup();
    }
}

} // namespace

void Toolbar::Draw(EditorContext& ctx, EditorApp& app) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetFrameHeight() + (style.WindowPadding.y * 2.0f);

    if (!ImGui::BeginChild("##EditorToolbar", ImVec2(0.0f, height), ImGuiChildFlags_Borders,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::EndChild();
        return;
    }

    // Playtest controls. Scene/game view state remains live-editable while the
    // gameplay preview runs.
    if (EditorIconToggleButton(EditorIcon::Play, "Simulate Scene", ctx.playMode,
                               ctx.playMode ? "Stop scene simulation" : "Simulate Scene in the Game viewport")) {
        app.TogglePlaytestMode();
    }
    ImGui::SameLine();
    if (EditorIconToggleButton(EditorIcon::Play,
                               "Play Client",
                               ctx.clientPreview.previewEnabled,
                               ctx.clientPreview.previewEnabled ? "Stop the runtime client preview"
                                                                : "Play Client using the active client flow")) {
        app.ToggleClientPreviewMode();
    }

    VerticalSeparator();

    if (EditorIconButton(EditorIcon::Save, "Save", "Save the active scene")) {
        app.SaveScene();
    }
    ImGui::SameLine();
    if (EditorIconButton(EditorIcon::Validate, "Validate", "Run scene validation checks")) {
        app.ValidateActiveScene();
    }

    // Right-aligned editor scale setting + active scene name + dirty indicator.
    const char* sceneName = ctx.activeScene != nullptr ? ctx.activeScene->GetName().c_str() : "<no scene>";
    char label[256];
    std::snprintf(label, sizeof(label), "%s%s", sceneName, ctx.sceneDirty ? " *" : "");

    const float scaleWidth = EditorScaleControlWidth();
    const float textWidth = ImGui::CalcTextSize(label).x;
    const float rightGroupWidth = scaleWidth + style.ItemSpacing.x + textWidth;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > rightGroupWidth + style.ItemSpacing.x) {
        ImGui::SameLine(ImGui::GetCursorPosX() + (avail - rightGroupWidth - style.ItemSpacing.x));
    } else {
        ImGui::SameLine();
    }

    DrawEditorScaleControl(ctx, app);
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    EditorTooltip::ForLastItem(ctx.sceneDirty ? "Active scene has unsaved changes" : "Active scene");

    ImGui::EndChild();
}

} // namespace Hockey
