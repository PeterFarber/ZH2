#include "Hockey/Editor/Inspector/ComponentInspector.hpp"

#include <string>

#include <imgui.h>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Inspector/FieldDrawers.hpp"

namespace Hockey {

namespace {

// Identity-style components surfaced in the inspector header rather than as
// their own component sections.
bool IsHeaderComponent(const std::string& name) {
    return name == "IDComponent" || name == "NameComponent" || name == "ActiveComponent";
}

} // namespace

void ComponentInspector::Draw(EditorContext& context, Entity& entity) {
    Scene* scene = entity.GetScene();
    if (scene == nullptr) {
        return;
    }
    const UUID uuid = entity.GetUUID();

    // While no field is being actively edited, keep a fresh "before" snapshot so
    // the moment a drag begins we already hold the pre-edit state.
    if (!m_Editing) {
        m_PreEditEntity = uuid;
        m_PreEditSnapshot = EntitySnapshot::CaptureEntity(*scene, uuid);
    }

    const ComponentMetadata* pendingRemove = nullptr;
    const ComponentMetadata* pendingPaste = nullptr;
    bool committed = false;
    std::string committedLabel;

    for (const ComponentMetadata& metadata : ComponentRegistry::Get().All()) {
        if (IsHeaderComponent(metadata.name)) {
            continue;
        }
        if (!metadata.has || !metadata.has(entity)) {
            continue;
        }

        ImGui::PushID(metadata.name.c_str());

        const bool open = ImGui::CollapsingHeader(metadata.displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        if (ImGui::BeginPopupContextItem("##componentctx")) {
            if (ImGui::MenuItem("Copy Component")) {
                context.clipboard.CopyComponent(*scene, uuid, metadata.name);
            }
            const bool canPaste =
                context.clipboard.HasComponent() && context.clipboard.ComponentName() == metadata.name;
            if (ImGui::MenuItem("Paste Component Values", nullptr, false, canPaste)) {
                pendingPaste = &metadata;
            }
            if (metadata.removable) {
                ImGui::Separator();
                if (ImGui::MenuItem("Remove Component")) {
                    pendingRemove = &metadata;
                }
            }
            ImGui::EndPopup();
        }

        if (open) {
            void* data = metadata.getData ? metadata.getData(entity) : nullptr;
            for (const FieldMetadata& field : metadata.fields) {
                ImGui::PushID(field.name.c_str());
                const FieldDrawers::FieldEdit edit = FieldDrawers::Draw(field, data, context.assetManager);
                if (edit.started) {
                    m_Editing = true;
                }
                if (edit.changed) {
                    context.MarkDirty();
                }
                if (edit.committed) {
                    committed = true;
                    committedLabel = metadata.displayName;
                }
                ImGui::PopID();
            }
            if (metadata.fields.empty()) {
                ImGui::TextDisabled("(no editable fields)");
            }
        }

        ImGui::PopID();
    }

    if (committed && m_PreEditEntity == uuid) {
        std::string after = EntitySnapshot::CaptureEntity(*scene, uuid);
        if (!m_PreEditSnapshot.empty() && after != m_PreEditSnapshot) {
            context.undoRedo.Execute(EditorCommands::EditComponentField(uuid, committedLabel, m_PreEditSnapshot, after),
                                     context);
        }
        m_PreEditSnapshot = std::move(after);
    }

    // Reset the edit latch once nothing is actively being dragged so the
    // baseline snapshot refreshes next frame.
    if (!ImGui::IsAnyItemActive()) {
        m_Editing = false;
    }

    if (pendingPaste != nullptr) {
        context.undoRedo.Execute(
            EditorCommands::PasteComponent(*scene, uuid, pendingPaste->name, context.clipboard.ComponentYaml()),
            context);
    }
    if (pendingRemove != nullptr) {
        context.undoRedo.Execute(EditorCommands::RemoveComponent(*scene, uuid, pendingRemove->name), context);
    }
}

} // namespace Hockey
