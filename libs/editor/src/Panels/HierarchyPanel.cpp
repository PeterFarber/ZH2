#include "Hockey/Editor/Panels/HierarchyPanel.hpp"

#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"

namespace Hockey {

namespace {

constexpr const char* kEntityDragType = "HOCKEY_HIERARCHY_ENTITY";

void* NodeId(UUID id) {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(id.Value()));
}

} // namespace

HierarchyPanel::HierarchyPanel() : Panel(EditorPanelNames::kHierarchy) {}

void HierarchyPanel::BeginRename(UUID id, const std::string& currentName) {
    m_RenamingId = id;
    m_RenameFocus = false;
    m_RenameOriginal = currentName;
    std::snprintf(m_RenameBuffer, sizeof(m_RenameBuffer), "%s", currentName.c_str());
}

void HierarchyPanel::OnImGui(EditorContext& context) {
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    Scene* scene = context.activeScene;
    if (scene == nullptr) {
        ImGui::TextUnformatted("No active scene.");
        EndWindow();
        return;
    }

    // Drop stale ids (e.g. entities deleted elsewhere) before drawing.
    context.selection.Validate(*scene);

    // Window-wide prefab drop target (instantiate at scene root). Submitted before
    // the entity nodes so a node's own "drop as child" target overrides it when a
    // node is hovered.
    DrawRootDropTarget();

    for (const Entity& root : scene->GetRootEntities()) {
        DrawEntityNode(context, *scene, root);
    }

    // Click on empty space clears the selection.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        context.selection.Clear();
    }

    // Right-click on empty space: create a root entity.
    if (ImGui::BeginPopupContextWindow("##HierarchyContext",
                                       ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty")) {
            m_PendingKind = PendingKind::CreateEmpty;
        }
        ImGui::EndPopup();
    }

    ApplyPending(context, *scene);

    EndWindow();
}

void HierarchyPanel::DrawEntityNode(EditorContext& context, Scene& scene, const Entity& entity) {
    Selection& selection = context.selection;
    const UUID id = entity.GetUUID();
    const std::vector<Entity> children = scene.GetChildren(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selection.IsSelected(id)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const bool renaming = m_RenamingId == id;
    bool open = false;

    if (renaming) {
        open = ImGui::TreeNodeEx(NodeId(id), flags, "%s", "");
        ImGui::SameLine();
        if (!m_RenameFocus) {
            ImGui::SetKeyboardFocusHere();
            m_RenameFocus = true;
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool committed =
            ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        if (committed || ImGui::IsItemDeactivated()) {
            if (Entity target = scene.FindEntityByUUID(id); target && m_RenameBuffer != m_RenameOriginal) {
                context.undoRedo.Execute(EditorCommands::RenameEntity(id, m_RenameOriginal, m_RenameBuffer), context);
            }
            m_RenamingId = UUID(0);
            m_RenameFocus = false;
        }
    } else {
        open = ImGui::TreeNodeEx(NodeId(id), flags, "%s", entity.GetName().c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            if (ImGui::GetIO().KeyCtrl) {
                selection.Toggle(id);
            } else {
                selection.Select(id);
            }
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            BeginRename(id, entity.GetName());
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            const std::uint64_t value = id.Value();
            ImGui::SetDragDropPayload(kEntityDragType, &value, sizeof(value));
            ImGui::TextUnformatted(entity.GetName().c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kEntityDragType)) {
                const std::uint64_t value = *static_cast<const std::uint64_t*>(payload->Data);
                m_PendingKind = PendingKind::Reparent;
                m_PendingTarget = UUID(value);
                m_PendingParent = id;
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropType)) {
                m_PendingKind = PendingKind::InstantiatePrefab;
                m_PendingPrefabPath = std::filesystem::path(static_cast<const char*>(payload->Data));
                m_PendingParent = id; // instantiate as child of the hovered entity
            }
            // Drop a Mesh/Material asset onto an entity to assign it on the
            // MeshRendererComponent (adding the component if necessary).
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropType)) {
                const auto* dropped = static_cast<const AssetDragPayload*>(payload->Data);
                if (Entity target = scene.FindEntityByUUID(id)) {
                    MeshRendererComponent& mr = target.HasComponent<MeshRendererComponent>()
                                                    ? target.GetComponent<MeshRendererComponent>()
                                                    : target.AddComponent<MeshRendererComponent>();
                    const AssetType type = static_cast<AssetType>(dropped->type);
                    if (type == AssetType::Material) {
                        mr.materialAsset = dropped->id;
                        context.MarkDirty();
                    } else if (type == AssetType::Mesh) {
                        mr.meshAsset = dropped->id;
                        context.MarkDirty();
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem()) {
            if (!selection.IsSelected(id)) {
                selection.Select(id);
            }
            if (ImGui::MenuItem("Create Empty")) {
                m_PendingKind = PendingKind::CreateEmpty;
            }
            if (ImGui::MenuItem("Create Empty Child")) {
                m_PendingKind = PendingKind::CreateChild;
                m_PendingTarget = id;
            }
            if (ImGui::MenuItem("Duplicate")) {
                m_PendingKind = PendingKind::Duplicate;
                m_PendingTarget = id;
            }
            if (ImGui::MenuItem("Rename")) {
                BeginRename(id, entity.GetName());
            }
            const bool hasParent = static_cast<bool>(scene.GetParent(entity));
            if (ImGui::MenuItem("Unparent", nullptr, false, hasParent)) {
                m_PendingKind = PendingKind::Unparent;
                m_PendingTarget = id;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy")) {
                context.clipboard.CopyEntities(scene, {id});
            }
            if (ImGui::MenuItem("Paste Child", nullptr, false, context.clipboard.HasEntities())) {
                for (const std::string& snapshot : context.clipboard.EntitySnapshots()) {
                    context.undoRedo.Execute(EditorCommands::PasteEntity(snapshot, id), context);
                }
            }
            if (ImGui::MenuItem("Create Prefab From Entity")) {
                m_PendingKind = PendingKind::CreatePrefab;
                m_PendingTarget = id;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                m_PendingKind = PendingKind::Delete;
                m_PendingTarget = id;
            }
            ImGui::EndPopup();
        }
    }

    if (open) {
        for (const Entity& child : children) {
            DrawEntityNode(context, scene, child);
        }
        ImGui::TreePop();
    }
}

void HierarchyPanel::DrawRootDropTarget() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr) {
        return;
    }
    if (ImGui::BeginDragDropTargetCustom(window->InnerRect, window->ID)) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropType)) {
            m_PendingKind = PendingKind::InstantiatePrefab;
            m_PendingPrefabPath = std::filesystem::path(static_cast<const char*>(payload->Data));
            m_PendingParent = UUID(0); // scene root
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::ApplyPending(EditorContext& context, Scene& scene) {
    const PendingKind kind = m_PendingKind;
    const UUID target = m_PendingTarget;
    const UUID parent = m_PendingParent;
    const std::filesystem::path prefabPath = m_PendingPrefabPath;
    m_PendingKind = PendingKind::None;
    m_PendingTarget = UUID(0);
    m_PendingParent = UUID(0);
    m_PendingPrefabPath.clear();

    switch (kind) {
    case PendingKind::None:
        return;
    case PendingKind::CreateEmpty:
        context.undoRedo.Execute(EditorCommands::CreateEntity("GameObject"), context);
        break;
    case PendingKind::CreateChild:
        context.undoRedo.Execute(EditorCommands::CreateEntity("GameObject", target), context);
        break;
    case PendingKind::Duplicate:
        if (scene.FindEntityByUUID(target)) {
            context.undoRedo.Execute(EditorCommands::DuplicateEntity(target), context);
        }
        break;
    case PendingKind::Delete:
        if (scene.FindEntityByUUID(target)) {
            context.undoRedo.Execute(EditorCommands::DeleteEntity(scene, target), context);
        }
        break;
    case PendingKind::Unparent:
        if (scene.FindEntityByUUID(target)) {
            context.undoRedo.Execute(EditorCommands::SetParent(scene, target, UUID(0)), context);
        }
        break;
    case PendingKind::Reparent: {
        Entity child = scene.FindEntityByUUID(target);
        Entity newParent = scene.FindEntityByUUID(parent);
        // Cannot parent an entity to itself or to one of its own descendants.
        if (child && newParent && child != newParent && !scene.IsDescendantOf(newParent, child)) {
            context.undoRedo.Execute(EditorCommands::SetParent(scene, target, parent), context);
        }
        break;
    }
    case PendingKind::InstantiatePrefab:
        if (!prefabPath.empty()) {
            const UUID parentId = scene.FindEntityByUUID(parent) ? parent : UUID(0);
            context.undoRedo.Execute(EditorCommands::InstantiatePrefab(prefabPath, parentId), context);
        }
        break;
    case PendingKind::CreatePrefab:
        if (Entity entity = scene.FindEntityByUUID(target)) {
            const std::filesystem::path dir = Paths::Get().rawAssets / "prefabs";
            const std::filesystem::path path = dir / (entity.GetName() + ".prefab.yaml");
            if (const Status status = PrefabSerializer::Save(scene, entity, path); status) {
                HK_EDITOR_INFO("Created prefab '{}'", path.string());
            } else {
                HK_EDITOR_ERROR("Create prefab failed: {}", status.error);
            }
        }
        break;
    }
}

} // namespace Hockey
