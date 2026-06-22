#include "Hockey/Editor/Panels/HierarchyPanel.hpp"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <imgui.h>

#include <imgui_internal.h>

#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"

namespace Hockey {

namespace {

constexpr const char* kEntityDragType = "HOCKEY_HIERARCHY_ENTITY";

enum class EntityDropPlacement {
    Before,
    Inside,
    After,
};

constexpr float kHierarchyDropLineThickness = 2.5f;
constexpr float kHierarchyDropHandleRadius = 3.0f;

void* NodeId(UUID id) {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(id.Value()));
}

EntityDropPlacement DropPlacementForCurrentItem() {
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float height = std::max(max.y - min.y, 1.0f);
    const float band = height / 3.0f;
    const float y = ImGui::GetMousePos().y;
    if (y < min.y + band) {
        return EntityDropPlacement::Before;
    }
    if (y > max.y - band) {
        return EntityDropPlacement::After;
    }
    return EntityDropPlacement::Inside;
}

UUID ParentIdOf(Scene& scene, const Entity& entity) {
    if (Entity parent = scene.GetParent(entity)) {
        return parent.GetUUID();
    }
    return UUID(0);
}

std::size_t AdjustSiblingIndexForRemoval(Scene& scene, UUID movedId, UUID newParentId, std::size_t siblingIndex) {
    Entity moved = scene.FindEntityByUUID(movedId);
    if (!moved) {
        return siblingIndex;
    }

    if (ParentIdOf(scene, moved) != newParentId) {
        return siblingIndex;
    }

    const std::size_t oldIndex = scene.GetSiblingIndex(moved);
    if (oldIndex < siblingIndex) {
        return siblingIndex - 1;
    }
    return siblingIndex;
}

bool IsEntityDropValid(Scene& scene, UUID movedId, const Entity& target, EntityDropPlacement placement) {
    Entity moved = scene.FindEntityByUUID(movedId);
    if (!moved || !target || moved == target) {
        return false;
    }

    Entity newParent;
    if (placement == EntityDropPlacement::Inside) {
        newParent = target;
    } else {
        newParent = scene.GetParent(target);
    }

    return !newParent || (moved != newParent && !scene.IsDescendantOf(newParent, moved));
}

void DrawHierarchyDropPreview(const ImRect& rowRect, EntityDropPlacement placement) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (draw == nullptr) {
        return;
    }

    const ImU32 lineColor = ImGui::GetColorU32(ImVec4(1.0f, 0.72f, 0.18f, 0.95f));
    const ImU32 insideFill = ImGui::GetColorU32(ImVec4(0.18f, 0.74f, 0.46f, 0.22f));
    const ImU32 insideBorder = ImGui::GetColorU32(ImVec4(0.26f, 0.96f, 0.62f, 0.95f));

    if (placement == EntityDropPlacement::Inside) {
        draw->AddRectFilled(rowRect.Min, rowRect.Max, insideFill, 3.0f);
        draw->AddRect(rowRect.Min, rowRect.Max, insideBorder, 3.0f, 0, 1.5f);
        return;
    }

    const float y = placement == EntityDropPlacement::Before ? rowRect.Min.y : rowRect.Max.y;
    const ImVec2 start(rowRect.Min.x + 2.0f, y);
    const ImVec2 end(rowRect.Max.x - 2.0f, y);
    draw->AddLine(start, end, lineColor, kHierarchyDropLineThickness);
    draw->AddCircleFilled(start, kHierarchyDropHandleRadius, lineColor);
    draw->AddCircleFilled(end, kHierarchyDropHandleRadius, lineColor);
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

    // Window-wide drop target for moving entities to scene root or instantiating
    // prefabs at scene root. Entity rows use smaller targets, so hovered rows win.
    DrawRootDropTarget(*scene);

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
        EditorTooltip::ForLastItem("Creates a new root entity in the active scene.");
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
        EditorTooltip::ForLastItem("Renames this entity when editing finishes.");
        if (committed || ImGui::IsItemDeactivated()) {
            if (Entity target = scene.FindEntityByUUID(id); target && m_RenameBuffer != m_RenameOriginal) {
                context.undoRedo.Execute(EditorCommands::RenameEntity(id, m_RenameOriginal, m_RenameBuffer), context);
            }
            m_RenamingId = UUID(0);
            m_RenameFocus = false;
        }
    } else {
        open = ImGui::TreeNodeEx(NodeId(id), flags, "%s", entity.GetName().c_str());
        const ImRect rowRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        EditorTooltip::ForLastItem("Drag to reorder or parent this entity. Double-click to rename.");

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
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(kEntityDragType, ImGuiDragDropFlags_AcceptPeekOnly)) {
                const std::uint64_t value = *static_cast<const std::uint64_t*>(payload->Data);
                const UUID movedId(value);
                const EntityDropPlacement placement = DropPlacementForCurrentItem();
                if (IsEntityDropValid(scene, movedId, entity, placement)) {
                    DrawHierarchyDropPreview(rowRect, placement);
                    if (payload->IsDelivery()) {
                        m_PendingKind = PendingKind::MoveEntity;
                        m_PendingTarget = movedId;
                        if (placement == EntityDropPlacement::Inside) {
                            m_PendingParent = id;
                            m_PendingSiblingIndex =
                                AdjustSiblingIndexForRemoval(scene, movedId, id, scene.GetChildren(entity).size());
                        } else {
                            const UUID parentId = ParentIdOf(scene, entity);
                            std::size_t siblingIndex = scene.GetSiblingIndex(entity);
                            if (placement == EntityDropPlacement::After) {
                                ++siblingIndex;
                            }
                            m_PendingParent = parentId;
                            m_PendingSiblingIndex =
                                AdjustSiblingIndexForRemoval(scene, movedId, parentId, siblingIndex);
                        }
                    }
                }
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
            EditorTooltip::ForLastItem("Creates a new root entity in the active scene.");
            if (ImGui::MenuItem("Create Empty Child")) {
                m_PendingKind = PendingKind::CreateChild;
                m_PendingTarget = id;
            }
            EditorTooltip::ForLastItem("Creates a child under the selected entity.");
            if (ImGui::MenuItem("Duplicate")) {
                m_PendingKind = PendingKind::Duplicate;
                m_PendingTarget = id;
            }
            EditorTooltip::ForLastItem("Duplicates this entity and its child hierarchy.");
            if (ImGui::MenuItem("Rename")) {
                BeginRename(id, entity.GetName());
            }
            EditorTooltip::ForLastItem("Starts inline rename for this hierarchy row.");
            const bool hasParent = static_cast<bool>(scene.GetParent(entity));
            if (ImGui::MenuItem("Unparent", nullptr, false, hasParent)) {
                m_PendingKind = PendingKind::Unparent;
                m_PendingTarget = id;
            }
            EditorTooltip::ForLastItem("Moves this entity to the scene root while preserving its world transform.");
            if (ImGui::MenuItem("Focus In Viewport", "F", false, entity.HasComponent<TransformComponent>())) {
                const glm::mat4 world = scene.GetWorldTransform(entity);
                const glm::vec3 position{world[3]};
                const float sx = glm::length(glm::vec3(world[0]));
                const float sy = glm::length(glm::vec3(world[1]));
                const float sz = glm::length(glm::vec3(world[2]));
                const float radius = std::max({sx, sy, sz, 1.0f}) * 0.75f;
                context.editorCamera.Focus(position, radius);
            }
            EditorTooltip::ForLastItem("Frames this entity in the Scene viewport camera.");
            ImGui::Separator();
            if (ImGui::MenuItem("Copy")) {
                context.clipboard.CopyEntities(scene, {id});
            }
            EditorTooltip::ForLastItem("Copies this entity hierarchy to the editor clipboard.");
            if (ImGui::MenuItem("Paste Child", nullptr, false, context.clipboard.HasEntities())) {
                for (const std::string& snapshot : context.clipboard.EntitySnapshots()) {
                    context.undoRedo.Execute(EditorCommands::PasteEntity(snapshot, id), context);
                }
            }
            EditorTooltip::ForLastItem("Pastes copied entities as children of this entity.");
            if (ImGui::MenuItem("Create Prefab From Entity")) {
                m_PendingKind = PendingKind::CreatePrefab;
                m_PendingTarget = id;
            }
            EditorTooltip::ForLastItem("Writes this entity hierarchy as a prefab asset.");
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                m_PendingKind = PendingKind::Delete;
                m_PendingTarget = id;
            }
            EditorTooltip::ForLastItem("Deletes this entity hierarchy with undo support.");
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

void HierarchyPanel::DrawRootDropTarget(Scene& scene) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr) {
        return;
    }
    if (ImGui::BeginDragDropTargetCustom(window->InnerRect, window->ID)) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(kEntityDragType, ImGuiDragDropFlags_AcceptPeekOnly)) {
            const std::uint64_t value = *static_cast<const std::uint64_t*>(payload->Data);
            const UUID movedId(value);
            if (scene.FindEntityByUUID(movedId)) {
                ImRect rootPreview = window->InnerRect;
                rootPreview.Min.y = std::max(rootPreview.Min.y, rootPreview.Max.y - ImGui::GetFrameHeight());
                DrawHierarchyDropPreview(rootPreview, EntityDropPlacement::After);
                if (payload->IsDelivery()) {
                    m_PendingKind = PendingKind::MoveEntity;
                    m_PendingTarget = movedId;
                    m_PendingParent = UUID(0);
                    m_PendingSiblingIndex =
                        AdjustSiblingIndexForRemoval(scene, movedId, UUID(0), scene.GetRootEntities().size());
                }
            }
        }
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
    const std::size_t siblingIndex = m_PendingSiblingIndex;
    const std::filesystem::path prefabPath = m_PendingPrefabPath;
    m_PendingKind = PendingKind::None;
    m_PendingTarget = UUID(0);
    m_PendingParent = UUID(0);
    m_PendingSiblingIndex = 0;
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
    case PendingKind::MoveEntity: {
        Entity child = scene.FindEntityByUUID(target);
        Entity newParent;
        if (parent.IsValid()) {
            newParent = scene.FindEntityByUUID(parent);
        }
        // Cannot parent an entity to itself or to one of its own descendants. Invalid
        // parent id means scene root.
        if (child && (!parent.IsValid() || (newParent && child != newParent && !scene.IsDescendantOf(newParent, child)))) {
            context.undoRedo.Execute(EditorCommands::MoveEntity(scene, target, parent, siblingIndex), context);
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
            context.undoRedo.Execute(EditorCommands::CreatePrefab(entity.GetUUID(), path), context);
            HK_EDITOR_INFO("Created prefab '{}'", path.string());
        }
        break;
    }
}

} // namespace Hockey
