#include "Hockey/Editor/Toolbar.hpp"

#include <cstdio>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {

namespace {

bool ToggleButton(const char* label, bool active, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    const bool clicked = ImGui::Button(label, size);
    if (active) {
        ImGui::PopStyleColor();
    }
    return clicked;
}

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
    if (ToggleButton("Play", ctx.playMode)) {
        app.TogglePlaytestMode();
    }
    EditorTooltip::ForLastItem(ctx.playMode ? "Stop the live gameplay preview" : "Start a live gameplay preview");

    VerticalSeparator();

    if (ImGui::Button("Save")) {
        app.SaveScene();
    }
    EditorTooltip::ForLastItem("Save the active scene");
    ImGui::SameLine();
    if (ImGui::Button("Validate")) {
        app.ValidateActiveScene();
    }
    EditorTooltip::ForLastItem("Run scene validation checks");

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
