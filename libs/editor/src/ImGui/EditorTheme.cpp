#include "Hockey/Editor/ImGui/EditorTheme.hpp"

#include <imgui.h>

namespace Hockey::EditorTheme {

void ApplyDark() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 18.0f;
    style.WindowMenuButtonPosition = ImGuiDir_None;

    ImVec4* colors = style.Colors;
    const ImVec4 bg = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    const ImVec4 bgLight = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    const ImVec4 bgLighter = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    const ImVec4 accent = ImVec4(0.26f, 0.50f, 0.85f, 1.00f);
    const ImVec4 accentHover = ImVec4(0.32f, 0.57f, 0.92f, 1.00f);
    const ImVec4 text = ImVec4(0.88f, 0.88f, 0.90f, 1.00f);

    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_ChildBg] = bg;
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.15f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.29f, 0.60f);
    colors[ImGuiCol_FrameBg] = bgLight;
    colors[ImGuiCol_FrameBgHovered] = bgLighter;
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.27f, 0.31f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = bg;
    colors[ImGuiCol_ScrollbarGrab] = bgLighter;
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_Button] = bgLighter;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accent;
    colors[ImGuiCol_Header] = bgLighter;
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_Separator] = ImVec4(0.26f, 0.26f, 0.29f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accentHover;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.26f, 0.29f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive] = accentHover;
    colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabSelected] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_NavCursor] = accent;
}

} // namespace Hockey::EditorTheme
