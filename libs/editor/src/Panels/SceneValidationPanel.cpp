#include "Hockey/Editor/Panels/SceneValidationPanel.hpp"

#include <cstddef>
#include <string>

#include <imgui.h>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {

void SceneValidationPanel::Validate(EditorContext& context) {
    if (context.activeScene == nullptr) {
        m_Issues.clear();
        m_HasRun = true;
        return;
    }
    m_Issues = SceneValidator::Validate(*context.activeScene);
    m_HasRun = true;
}

SceneValidationPanel::SceneValidationPanel() : Panel(EditorPanelNames::kSceneValidation) {}

void SceneValidationPanel::OnImGui(EditorContext& context) {
    if (BeginWindow()) {
        if (m_AutoValidate && context.activeScene != nullptr) {
            Validate(context);
        }

        if (ImGui::Button("Validate")) {
            Validate(context);
        }
        EditorTooltip::ForLastItem("Runs scene validation for required hockey markers and authored scene data.");
        ImGui::SameLine();
        ImGui::Checkbox("Auto", &m_AutoValidate);
        EditorTooltip::ForLastItem("Runs validation automatically while this panel is visible.");

        std::size_t errors = 0;
        std::size_t warnings = 0;
        for (const SceneValidationIssue& issue : m_Issues) {
            if (issue.severity == SceneValidationIssue::Severity::Error) {
                ++errors;
            } else {
                ++warnings;
            }
        }

        ImGui::SameLine();
        ImGui::TextColored(errors > 0 ? ImVec4(1.0f, 0.45f, 0.42f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%zu errors",
                           errors);
        EditorTooltip::ForLastItem("Errors block a valid authored scene until fixed.");
        ImGui::SameLine();
        ImGui::TextColored(warnings > 0 ? ImVec4(1.0f, 0.82f, 0.36f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                           "%zu warnings", warnings);
        EditorTooltip::ForLastItem("Warnings flag risky or incomplete scene data.");

        ImGui::Separator();

        if (!m_HasRun) {
            ImGui::TextDisabled("Press Validate to check the active scene.");
        } else if (m_Issues.empty()) {
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.55f, 1.0f), "No issues found.");
        } else {
            if (ImGui::BeginChild("##validation-list", ImVec2(0.0f, 0.0f), false)) {
                Scene* scene = context.activeScene;
                int index = 0;
                for (const SceneValidationIssue& issue : m_Issues) {
                    const bool isError = issue.severity == SceneValidationIssue::Severity::Error;
                    const ImVec4 color = isError ? ImVec4(1.0f, 0.45f, 0.42f, 1.0f) : ImVec4(1.0f, 0.82f, 0.36f, 1.0f);

                    std::string label = isError ? "[ERROR] " : "[WARN]  ";
                    label += issue.message;
                    if (issue.entityId.IsValid() && scene != nullptr) {
                        if (Entity entity = scene->FindEntityByUUID(issue.entityId)) {
                            label += "  (" + entity.GetName() + ")";
                        }
                    }
                    label += "##" + std::to_string(index++);

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    const bool clicked = ImGui::Selectable(label.c_str());
                    EditorTooltip::ForLastItem("Selects the related entity when this validation issue has one.");
                    ImGui::PopStyleColor();
                    if (clicked && issue.entityId.IsValid()) {
                        context.selection.Select(issue.entityId);
                    }
                }
            }
            ImGui::EndChild();
        }
    }
    EndWindow();
}

} // namespace Hockey
