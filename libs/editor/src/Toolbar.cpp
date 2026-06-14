#include "Hockey/Editor/Toolbar.hpp"

#include <cstdio>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorContext.hpp"

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

    // Transform tools. Activation routes through the ToolManager, which sets the
    // viewport gizmo mode; the active tool is highlighted.
    const char* const transformTools[] = {"Select", "Move", "Rotate", "Scale"};
    for (const char* toolName : transformTools) {
        if (toolName != transformTools[0]) {
            ImGui::SameLine();
        }
        const EditorTool* tool = ctx.toolManager.Find(toolName);
        if (ToggleButton(toolName, tool != nullptr && ctx.toolManager.IsActive(tool))) {
            ctx.toolManager.Activate(toolName, ctx);
        }
    }

    VerticalSeparator();

    // Transform space toggle.
    const bool worldSpace = ctx.transformSpace == TransformSpace::World;
    if (ImGui::Button(worldSpace ? "World" : "Local")) {
        ctx.transformSpace = worldSpace ? TransformSpace::Local : TransformSpace::World;
    }

    ImGui::SameLine();
    if (ToggleButton("Snap", ctx.settings.snapEnabled)) {
        ctx.settings.snapEnabled = !ctx.settings.snapEnabled;
    }
    ImGui::SameLine();
    if (ToggleButton("Grid", ctx.settings.showGrid)) {
        ctx.settings.showGrid = !ctx.settings.showGrid;
    }

    VerticalSeparator();

    // Play-mode controls (scene lifecycle only; no gameplay in Phase 4).
    if (ToggleButton("Play", ctx.playMode)) {
        app.TogglePlayMode();
    }
    ImGui::SameLine();
    if (ToggleButton("Simulate", ctx.simulateMode)) {
        app.ToggleSimulateMode();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::Button("Pause");
    ImGui::SameLine();
    ImGui::Button("Step");
    ImGui::EndDisabled();

    VerticalSeparator();

    if (ImGui::Button("Save")) {
        app.SaveScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate")) {
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

    ImGui::EndChild();
}

} // namespace Hockey
