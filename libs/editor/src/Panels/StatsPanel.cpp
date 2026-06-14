#include "Hockey/Editor/Panels/StatsPanel.hpp"

#include <cstdarg>
#include <cstddef>

#include <imgui.h>

#include "Hockey/Core/Window.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

void Row(const char* label, const char* fmt, ...) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

bool BeginStatsTable(const char* id) {
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable(id, 2, flags)) {
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        return true;
    }
    return false;
}

} // namespace

StatsPanel::StatsPanel() : Panel(EditorPanelNames::kStats) {}

void StatsPanel::OnImGui(EditorContext& context) {
    if (BeginWindow()) {
        const ImGuiIO& io = ImGui::GetIO();
        const float fps = io.Framerate;
        const float frameMs = fps > 0.0f ? 1000.0f / fps : 0.0f;

        if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen) && BeginStatsTable("##frame")) {
            Row("FPS", "%.1f", fps);
            Row("Frame time", "%.2f ms", frameMs);
            if (context.renderer != nullptr) {
                const RendererStats& stats = context.renderer->GetStats();
                Row("CPU frame", "%.2f ms", stats.cpuFrameMs);
                if (stats.gpuFrameMs > 0.0f) {
                    Row("GPU frame", "%.2f ms", stats.gpuFrameMs);
                } else {
                    Row("GPU frame", "n/a");
                }
            }
            ImGui::EndTable();
        }

        if (context.renderer != nullptr && ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen) &&
            BeginStatsTable("##renderer")) {
            const RendererStats& stats = context.renderer->GetStats();
            Row("Draw calls", "%u", stats.drawCalls);
            Row("Triangles", "%u", stats.triangleCount);
            Row("Shadow draw calls", "%u", stats.shadowDrawCalls);
            Row("Meshes", "%u", stats.meshCount);
            Row("Materials", "%u", stats.materialCount);
            Row("Textures", "%u", stats.textureCount);
            if (stats.budgetVRAMBytes > 0) {
                const double usedMb = static_cast<double>(stats.usedVRAMBytes) / (1024.0 * 1024.0);
                const double budgetMb = static_cast<double>(stats.budgetVRAMBytes) / (1024.0 * 1024.0);
                Row("VRAM", "%.1f / %.1f MB", usedMb, budgetMb);
            } else if (stats.usedVRAMBytes > 0) {
                Row("VRAM", "%.1f MB", static_cast<double>(stats.usedVRAMBytes) / (1024.0 * 1024.0));
            } else {
                Row("VRAM", "n/a");
            }
            if (context.window != nullptr) {
                Row("Renderer resolution", "%u x %u", context.window->Width(), context.window->Height());
            }
            if (context.viewportWidth > 0 && context.viewportHeight > 0) {
                Row("Viewport resolution", "%u x %u", context.viewportWidth, context.viewportHeight);
            } else {
                Row("Viewport resolution", "n/a");
            }
            ImGui::EndTable();
        }

        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen) && BeginStatsTable("##scene")) {
            const std::size_t entityCount = context.activeScene != nullptr ? context.activeScene->EntityCount() : 0;
            Row("Entities", "%zu", entityCount);
            Row("Selected", "%zu", context.selection.Count());
            ImGui::EndTable();
        }
    }
    EndWindow();
}

} // namespace Hockey
