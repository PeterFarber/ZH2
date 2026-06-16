#include "Hockey/Editor/Panels/PrefabPanel.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

#include <imgui.h>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/FileDialog.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

namespace Hockey {

namespace {

std::string SanitizeFileStem(std::string name) {
    for (char& ch : name) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' ||
            ch == '|' || ch == ' ') {
            ch = '_';
        }
    }
    if (name.empty()) {
        name = "Prefab";
    }
    return name;
}

} // namespace

PrefabPanel::PrefabPanel() : Panel(EditorPanelNames::kPrefab, /*openByDefault=*/false) {}

void PrefabPanel::RefreshOverrides(EditorContext& context, Entity entity) {
    m_OverrideEntityId = entity.GetUUID();
    m_OverrideCount = 0;
    m_OverrideValid = false;
    if (context.activeScene == nullptr || !entity.HasComponent<PrefabComponent>()) {
        return;
    }
    PrefabOverrideSet set;
    if (const Result<int> diff = PrefabSerializer::ComputeOverrides(*context.activeScene, entity, set); diff) {
        m_OverrideCount = diff.value;
        m_OverrideValid = true;
    }
}

void PrefabPanel::OnImGui(EditorContext& context) {
    if (BeginWindow()) {
        Scene* scene = context.activeScene;

        if (scene != nullptr) {
            if (ImGui::Button("Instantiate Prefab from File...")) {
                const std::filesystem::path dir = Paths::Get().rawAssets / "prefabs";
                if (const auto path = FileDialog::OpenFile({{"Hockey Prefab", "yaml"}}, dir)) {
                    context.undoRedo.Execute(EditorCommands::InstantiatePrefab(*path), context);
                    m_Status = "Instantiated " + path->filename().string();
                    m_StatusError = false;
                }
            }
            ImGui::Separator();
        }

        Entity entity = scene != nullptr ? scene->FindEntityByUUID(context.selection.Primary()) : Entity{};

        if (!entity) {
            ImGui::TextDisabled("Select an entity to view prefab options.");
            if (!m_Status.empty()) {
                ImGui::TextColored(m_StatusError ? ImVec4(1.0f, 0.45f, 0.42f, 1.0f) : ImVec4(0.55f, 0.85f, 0.55f, 1.0f),
                                   "%s", m_Status.c_str());
            }
            EndWindow();
            return;
        }

        ImGui::Text("Entity: %s", entity.GetName().c_str());
        ImGui::Separator();

        if (entity.HasComponent<PrefabComponent>()) {
            const auto& prefab = entity.GetComponent<PrefabComponent>();
            ImGui::TextColored(ImVec4(0.70f, 0.85f, 0.55f, 1.0f), "Prefab Instance");
            ImGui::TextWrapped("Source: %s", prefab.sourcePath.string().c_str());
            ImGui::Text("Asset ID: %s", prefab.prefabAssetId.ToString().c_str());
            ImGui::Text("Source Entity ID: %s", prefab.sourceEntityId.ToString().c_str());

            const bool sourceExists = !prefab.sourcePath.empty() && std::filesystem::exists(prefab.sourcePath);
            ImGui::BeginDisabled(!sourceExists);
            if (ImGui::Button("Open Prefab Source")) {
                ProjectBrowser::Reveal(prefab.sourcePath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Instantiate Copy")) {
                context.undoRedo.Execute(EditorCommands::InstantiatePrefab(prefab.sourcePath), context);
            }
            ImGui::EndDisabled();
            if (!sourceExists) {
                ImGui::SameLine();
                ImGui::TextDisabled("(source file missing)");
            }

            ImGui::Spacing();

            // Refresh the cached override diff when the inspected instance
            // changes (avoids re-reading the prefab file every frame).
            if (m_OverrideEntityId != entity.GetUUID()) {
                RefreshOverrides(context, entity);
            }
            if (m_OverrideValid) {
                if (m_OverrideCount == 0) {
                    ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.55f, 1.0f), "No overrides (matches prefab)");
                } else {
                    ImGui::TextColored(ImVec4(0.95f, 0.80f, 0.45f, 1.0f), "%d field(s) differ from prefab",
                                       m_OverrideCount);
                }
            } else {
                ImGui::TextDisabled("Override status unavailable (source missing?)");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Refresh")) {
                RefreshOverrides(context, entity);
            }

            ImGui::BeginDisabled(!sourceExists);
            if (ImGui::Button("Apply Overrides")) {
                if (const Status status = PrefabSerializer::ApplyInstanceToPrefab(*scene, entity); !status) {
                    m_Status = status.error;
                    m_StatusError = true;
                } else {
                    // Reimport so the asset database hash tracks the rewritten file
                    // (keeps hot-reload from flagging it as an external change).
                    if (context.assetManager != nullptr) {
                        context.assetManager->ImportAsset(prefab.sourcePath);
                        context.assetManager->SaveDatabase();
                    }
                    m_Status = "Applied overrides to " + prefab.sourcePath.filename().string();
                    m_StatusError = false;
                    context.MarkDirty();
                }
                RefreshOverrides(context, entity);
            }
            ImGui::SameLine();
            if (ImGui::Button("Revert Overrides")) {
                context.undoRedo.Execute(EditorCommands::RevertPrefabOverrides(*scene, entity.GetUUID()), context);
                m_Status = "Reverted to prefab";
                m_StatusError = false;
                RefreshOverrides(context, entity);
            }
            ImGui::EndDisabled();
            if (!sourceExists) {
                ImGui::TextDisabled("Apply/Revert need the prefab source file.");
            }
        } else {
            ImGui::TextDisabled("Not a prefab instance.");
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Create prefab from this entity:");
        if (m_NameBuffer[0] == '\0') {
            std::snprintf(m_NameBuffer, sizeof(m_NameBuffer), "%s", entity.GetName().c_str());
        }
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputText("Name", m_NameBuffer, sizeof(m_NameBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Create Prefab")) {
            const std::string stem = SanitizeFileStem(m_NameBuffer);
            const std::filesystem::path path = Paths::Get().rawAssets / "prefabs" / (stem + ".prefab.yaml");
            context.undoRedo.Execute(EditorCommands::CreatePrefab(entity.GetUUID(), path), context);
            if (std::filesystem::exists(path)) {
                m_Status = "Saved " + path.filename().string();
                m_StatusError = false;
            } else {
                m_Status = "Create prefab failed (see Console)";
                m_StatusError = true;
            }
        }

        if (!m_Status.empty()) {
            ImGui::TextColored(m_StatusError ? ImVec4(1.0f, 0.45f, 0.42f, 1.0f) : ImVec4(0.55f, 0.85f, 0.55f, 1.0f),
                               "%s", m_Status.c_str());
        }
    }
    EndWindow();
}

} // namespace Hockey
