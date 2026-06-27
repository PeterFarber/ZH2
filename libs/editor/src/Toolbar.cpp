#include "Hockey/Editor/Toolbar.hpp"

#include <cstdio>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {

namespace {

void VerticalSeparator() {
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
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

    // Right-aligned active scene name + dirty indicator.
    const char* sceneName = ctx.activeScene != nullptr ? ctx.activeScene->GetName().c_str() : "<no scene>";
    char label[256];
    std::snprintf(label, sizeof(label), "%s%s", sceneName, ctx.sceneDirty ? " *" : "");
    const float textWidth = ImGui::CalcTextSize(label).x;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > textWidth + style.ItemSpacing.x) {
        ImGui::SameLine(ImGui::GetCursorPosX() + (avail - textWidth - style.ItemSpacing.x));
    } else {
        ImGui::SameLine();
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    EditorTooltip::ForLastItem(ctx.sceneDirty ? "Active scene has unsaved changes" : "Active scene");

    ImGui::EndChild();
}

} // namespace Hockey
