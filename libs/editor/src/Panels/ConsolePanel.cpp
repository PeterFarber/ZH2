#include "Hockey/Editor/Panels/ConsolePanel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {

namespace {

ImVec4 ColorForLevel(int levelValue) {
    switch (levelValue) {
    case 0: // trace
    case 1: // debug
        return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    case 3: // warn
        return ImVec4(1.0f, 0.82f, 0.36f, 1.0f);
    case 4: // error
    case 5: // critical
        return ImVec4(1.0f, 0.45f, 0.42f, 1.0f);
    default: // info
        return ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
    }
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string FormatLine(const EditorLogEntry& entry) {
    return "[" + entry.timestamp + "] [" + entry.loggerName + "/" + entry.level + "] " + entry.message;
}

} // namespace

ConsolePanel::ConsolePanel() : Panel(EditorPanelNames::kConsole) {}

bool ConsolePanel::PassesFilter(const EditorLogEntry& entry) const {
    switch (entry.levelValue) {
    case 0:
    case 1:
        if (!m_ShowTrace) {
            return false;
        }
        break;
    case 3:
        if (!m_ShowWarn) {
            return false;
        }
        break;
    case 4:
    case 5:
        if (!m_ShowError) {
            return false;
        }
        break;
    default:
        if (!m_ShowInfo) {
            return false;
        }
        break;
    }
    if (m_Search[0] != '\0') {
        const std::string needle = ToLower(m_Search);
        if (ToLower(entry.message).find(needle) == std::string::npos &&
            ToLower(entry.loggerName).find(needle) == std::string::npos) {
            return false;
        }
    }
    return true;
}

void ConsolePanel::DrawToolbar() {
    if (ImGui::Button("Clear")) {
        EditorConsoleStore::Get().Clear();
        m_SelectedLine.clear();
    }
    EditorTooltip::ForLastItem("Clears all captured editor log entries.");
    ImGui::SameLine();
    if (ImGui::Button("Copy")) {
        if (!m_SelectedLine.empty()) {
            ImGui::SetClipboardText(m_SelectedLine.c_str());
        } else {
            std::string all;
            for (const EditorLogEntry& entry : m_Cached) {
                if (PassesFilter(entry)) {
                    all += FormatLine(entry);
                    all += '\n';
                }
            }
            ImGui::SetClipboardText(all.c_str());
        }
    }
    EditorTooltip::ForLastItem("Copies the selected log line, or all visible filtered lines if none is selected.");
    ImGui::SameLine();
    ImGui::Checkbox("Trace", &m_ShowTrace);
    EditorTooltip::ForLastItem("Shows trace and debug log messages.");
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_ShowInfo);
    EditorTooltip::ForLastItem("Shows informational log messages.");
    ImGui::SameLine();
    ImGui::Checkbox("Warn", &m_ShowWarn);
    EditorTooltip::ForLastItem("Shows warning log messages.");
    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_ShowError);
    EditorTooltip::ForLastItem("Shows error and critical log messages.");
    ImGui::SameLine();
    ImGui::Checkbox("Collapse", &m_Collapse);
    EditorTooltip::ForLastItem("Combines consecutive identical log lines into one visible entry.");
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
    EditorTooltip::ForLastItem("Scrolls to the newest log entry when new messages arrive.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputTextWithHint("##console-search", "Filter...", m_Search, sizeof(m_Search));
    EditorTooltip::ForLastItem("Filters visible logs by message text or logger name.");
}

void ConsolePanel::OnImGui(EditorContext& /*context*/) {
    if (BeginWindow()) {
        EditorConsoleStore& store = EditorConsoleStore::Get();
        const std::size_t revision = store.Revision();
        const bool newData = revision != m_CachedRevision;
        if (newData) {
            m_Cached = store.Snapshot();
            m_CachedRevision = revision;
        }

        DrawToolbar();
        ImGui::Separator();

        if (ImGui::BeginChild("##console-log", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            int index = 0;
            std::size_t collapsedRun = 0;
            for (std::size_t i = 0; i < m_Cached.size(); ++i) {
                const EditorLogEntry& entry = m_Cached[i];
                if (!PassesFilter(entry)) {
                    continue;
                }
                // Collapse consecutive identical lines (same level + message).
                if (m_Collapse && i + 1 < m_Cached.size()) {
                    const EditorLogEntry& next = m_Cached[i + 1];
                    if (next.levelValue == entry.levelValue && next.message == entry.message &&
                        next.loggerName == entry.loggerName) {
                        ++collapsedRun;
                        continue;
                    }
                }

                std::string label = FormatLine(entry);
                if (collapsedRun > 0) {
                    char suffix[32];
                    std::snprintf(suffix, sizeof(suffix), "  (x%zu)", collapsedRun + 1);
                    label += suffix;
                    collapsedRun = 0;
                }

                const std::string id = label + "##" + std::to_string(index++);
                ImGui::PushStyleColor(ImGuiCol_Text, ColorForLevel(entry.levelValue));
                if (ImGui::Selectable(id.c_str(), m_SelectedLine == label)) {
                    m_SelectedLine = label;
                }
                EditorTooltip::ForLastItem("Selects this log line so Copy captures only this entry.");
                ImGui::PopStyleColor();
            }

            if (newData && m_AutoScroll) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
    }
    EndWindow();
}

} // namespace Hockey
