#include "Hockey/Editor/Panels/HierarchyPanel.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
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
#include "Hockey/Editor/ImGui/EditorIcons.hpp"
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

bool ContainsId(const std::vector<UUID>& ids, UUID id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void CollectTopLevelSelectedInHierarchyOrder(Scene& scene, const Entity& entity,
                                             const std::unordered_set<std::uint64_t>& selectedIds,
                                             bool ancestorSelected, std::vector<UUID>& out) {
    if (!entity) {
        return;
    }

    const UUID id = entity.GetUUID();
    const bool selected = selectedIds.contains(id.Value());
    if (selected && !ancestorSelected) {
        out.push_back(id);
    }

    const bool blockDescendants = ancestorSelected || selected;
    for (const Entity& child : scene.GetChildren(entity)) {
        CollectTopLevelSelectedInHierarchyOrder(scene, child, selectedIds, blockDescendants, out);
    }
}

std::vector<UUID> ResolveDragMoveTargets(Scene& scene, const Selection& selection, UUID draggedId) {
    if (!draggedId.IsValid() || !scene.ContainsUUID(draggedId)) {
        return {};
    }
    if (!selection.IsSelected(draggedId)) {
        return {draggedId};
    }

    std::unordered_set<std::uint64_t> selectedIds;
    selectedIds.reserve(selection.All().size());
    for (const UUID id : selection.All()) {
        if (id.IsValid() && scene.ContainsUUID(id)) {
            selectedIds.insert(id.Value());
        }
    }

    std::vector<UUID> targets;
    targets.reserve(selectedIds.size());
    for (const Entity& root : scene.GetRootEntities()) {
        CollectTopLevelSelectedInHierarchyOrder(scene, root, selectedIds, /*ancestorSelected=*/false, targets);
    }

    return targets.empty() ? std::vector<UUID>{draggedId} : targets;
}

bool ShouldPreserveSelectionForPotentialDrag(const Selection& selection, UUID id) {
    return selection.Count() > 1 && selection.IsSelected(id);
}

std::string ToLower(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char c : text) {
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

std::string TrimmedSearchText(const char* text) {
    if (text == nullptr) {
        return {};
    }
    std::string value(text);
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

bool HierarchyNameMatchesSearch(const Entity& entity, std::string_view lowerNeedle) {
    if (lowerNeedle.empty()) {
        return true;
    }
    return ToLower(entity.GetName()).find(lowerNeedle) != std::string::npos;
}

bool BuildHierarchyFilterRecursive(Scene& scene, const Entity& entity, std::string_view lowerNeedle,
                                   std::unordered_set<std::uint64_t>& visibleIds) {
    bool visible = HierarchyNameMatchesSearch(entity, lowerNeedle);
    for (const Entity& child : scene.GetChildren(entity)) {
        visible = BuildHierarchyFilterRecursive(scene, child, lowerNeedle, visibleIds) || visible;
    }
    if (visible) {
        visibleIds.insert(entity.GetUUID().Value());
    }
    return visible;
}

bool BuildHierarchyFilter(Scene& scene, const char* searchText, std::unordered_set<std::uint64_t>& visibleIds) {
    visibleIds.clear();
    const std::string trimmed = TrimmedSearchText(searchText);
    if (trimmed.empty()) {
        return false;
    }

    const std::string lowerNeedle = ToLower(trimmed);
    for (const Entity& root : scene.GetRootEntities()) {
        BuildHierarchyFilterRecursive(scene, root, lowerNeedle, visibleIds);
    }
    return true;
}

void CollectVisibleHierarchyRows(Scene& scene, const Entity& entity,
                                 const std::unordered_set<std::uint64_t>* visibleFilter, bool searchActive,
                                 const std::unordered_set<std::uint64_t>& expandedIds, std::vector<UUID>& out) {
    const UUID id = entity.GetUUID();
    out.push_back(id);

    const bool open = searchActive || expandedIds.contains(id.Value());
    if (!open) {
        return;
    }

    for (const Entity& child : scene.GetChildren(entity)) {
        if (visibleFilter == nullptr || visibleFilter->contains(child.GetUUID().Value())) {
            CollectVisibleHierarchyRows(scene, child, visibleFilter, searchActive, expandedIds, out);
        }
    }
}

struct BranchSceneState {
    bool anySet = false;
    bool anyUnset = false;
};

BranchSceneState VisibilityState(EditorContext& context, Scene& scene, const Entity& entity) {
    BranchSceneState state;
    const bool hidden = context.IsSceneHidden(entity.GetUUID());
    state.anySet = hidden;
    state.anyUnset = !hidden;
    for (const Entity& child : scene.GetChildren(entity)) {
        const BranchSceneState childState = VisibilityState(context, scene, child);
        state.anySet = state.anySet || childState.anySet;
        state.anyUnset = state.anyUnset || childState.anyUnset;
    }
    return state;
}

BranchSceneState PickabilityState(EditorContext& context, Scene& scene, const Entity& entity) {
    BranchSceneState state;
    const bool unpickable = !context.IsScenePickable(entity.GetUUID());
    state.anySet = unpickable;
    state.anyUnset = !unpickable;
    for (const Entity& child : scene.GetChildren(entity)) {
        const BranchSceneState childState = PickabilityState(context, scene, child);
        state.anySet = state.anySet || childState.anySet;
        state.anyUnset = state.anyUnset || childState.anyUnset;
    }
    return state;
}

bool VisibilityMixed(EditorContext& context, Scene& scene, const Entity& entity) {
    const BranchSceneState state = VisibilityState(context, scene, entity);
    return state.anySet && state.anyUnset;
}

bool PickabilityMixed(EditorContext& context, Scene& scene, const Entity& entity) {
    const BranchSceneState state = PickabilityState(context, scene, entity);
    return state.anySet && state.anyUnset;
}

void DrawSceneVisibilityControl(EditorContext& context, Scene& scene, const Entity& entity) {
    const UUID id = entity.GetUUID();
    const bool hidden = context.IsSceneHidden(id);
    const bool mixed = VisibilityMixed(context, scene, entity);
    const EditorIcon icon = mixed ? EditorIcon::Warning : (hidden ? EditorIcon::Hidden : EditorIcon::Visible);
    const char* label = mixed ? "Mixed Visibility##visibility" : (hidden ? "Hidden##visibility" : "Visible##visibility");
    if (EditorIconButton(icon, label,
                         "Scene visibility: visible, hidden, or mixed. Alt-click affects only this entity.",
                         ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
        const bool includeDescendants = !ImGui::GetIO().KeyAlt;
        context.SetSceneHidden(scene, id, !hidden, includeDescendants);
    }
}

void DrawScenePickabilityControl(EditorContext& context, Scene& scene, const Entity& entity) {
    const UUID id = entity.GetUUID();
    const bool pickable = context.IsScenePickable(id);
    const bool mixed = PickabilityMixed(context, scene, entity);
    const EditorIcon icon = mixed ? EditorIcon::Warning : (pickable ? EditorIcon::Pickable : EditorIcon::Locked);
    const char* label = mixed ? "Mixed Pickability##pickability" : (pickable ? "Pickable##pickability" : "Locked##pickability");
    if (EditorIconButton(icon, label,
                         "Scene pickability: selectable, locked, or mixed. Alt-click affects only this entity.",
                         ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
        const bool includeDescendants = !ImGui::GetIO().KeyAlt;
        context.SetSceneUnpickable(scene, id, pickable, includeDescendants);
    }
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

bool IsEntityDropValid(Scene& scene, const std::vector<UUID>& movedIds, const Entity& target,
                       EntityDropPlacement placement) {
    if (movedIds.empty() || !target || ContainsId(movedIds, target.GetUUID())) {
        return false;
    }

    Entity newParent;
    if (placement == EntityDropPlacement::Inside) {
        newParent = target;
    } else {
        newParent = scene.GetParent(target);
    }

    for (const UUID movedId : movedIds) {
        Entity moved = scene.FindEntityByUUID(movedId);
        if (!moved) {
            return false;
        }
        if (newParent && (moved == newParent || scene.IsDescendantOf(newParent, moved))) {
            return false;
        }
    }
    return true;
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
    context.PruneHierarchyEditorState(*scene);
    DropStaleExpandedIds(*scene);

    const ImGuiIO& io = ImGui::GetIO();
    const bool hierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (hierarchyFocused && io.KeyCtrl && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        m_FocusSearchNextFrame = true;
    }
    if (hierarchyFocused && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_H, false)) {
        context.ToggleSelectedVisibility(*scene);
    }
    if (hierarchyFocused && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_L, false)) {
        context.ToggleSelectedPickability(*scene);
    }

    DrawSearchToolbar();

    // Window-wide drop target for moving entities to scene root or instantiating
    // prefabs at scene root. Entity rows use smaller targets, so hovered rows win.
    DrawRootDropTarget(context, *scene);

    std::unordered_set<std::uint64_t> visibleFilter;
    const bool searchActive = BuildHierarchyFilter(*scene, m_SearchBuffer.data(), visibleFilter);
    const auto* filter = searchActive ? &visibleFilter : nullptr;

    m_VisibleEntityRows.clear();
    for (const Entity& root : scene->GetRootEntities()) {
        if (filter == nullptr || filter->contains(root.GetUUID().Value())) {
            CollectVisibleHierarchyRows(*scene, root, filter, searchActive, m_ExpandedEntityIds, m_VisibleEntityRows);
        }
    }

    for (const Entity& root : scene->GetRootEntities()) {
        if (filter == nullptr || filter->contains(root.GetUUID().Value())) {
            DrawEntityNode(context, *scene, root, filter, searchActive);
        }
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
        EditorTooltip::ForLastItem("Creates a new entity under the default parent when one is set.");
        if (ImGui::MenuItem("Clear Default Parent", nullptr, false, context.HierarchyDefaultParent().IsValid())) {
            context.ClearHierarchyDefaultParent();
        }
        EditorTooltip::ForLastItem("Clears the hierarchy default parent.");
        ImGui::EndPopup();
    }

    ApplyPending(context, *scene);

    EndWindow();
}

void HierarchyPanel::DrawSearchToolbar() {
    if (m_FocusSearchNextFrame) {
        ImGui::SetKeyboardFocusHere();
        m_FocusSearchNextFrame = false;
    }

    const float clearWidth = ImGui::CalcTextSize("Clear Search").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - clearWidth - ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##hierarchy-search", "Search GameObjects", m_SearchBuffer.data(), m_SearchBuffer.size());
    EditorTooltip::ForLastItem("Filters hierarchy rows by entity name while keeping matching ancestors visible.");
    ImGui::SameLine();
    const bool hasSearch = TrimmedSearchText(m_SearchBuffer.data()).empty() == false;
    if (!hasSearch) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Clear Search", ImVec2(clearWidth, 0.0f))) {
        m_SearchBuffer[0] = '\0';
    }
    if (!hasSearch) {
        ImGui::EndDisabled();
    }
    EditorTooltip::ForLastItem("Clear Search");
    ImGui::Separator();
}

void HierarchyPanel::DrawEntityNode(EditorContext& context, Scene& scene, const Entity& entity,
                                    const std::unordered_set<std::uint64_t>* visibleFilter, bool searchActive) {
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

    const bool defaultParent = context.HierarchyDefaultParent() == id && context.DefaultParentFor(scene) == id;
    if (defaultParent) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.86f, 0.32f, 1.0f));
    }

    if (renaming) {
        ImGui::PushID(NodeId(id));
        DrawSceneVisibilityControl(context, scene, entity);
        ImGui::SameLine();
        DrawScenePickabilityControl(context, scene, entity);
        ImGui::PopID();
        ImGui::SameLine();
        if (!children.empty()) {
            ImGui::SetNextItemOpen(searchActive || IsEntityExpanded(id), ImGuiCond_Always);
        }
        open = ImGui::TreeNodeEx(NodeId(id), flags, "%s", "");
        if (!searchActive && ImGui::IsItemToggledOpen()) {
            if (ImGui::GetIO().KeyAlt) {
                SetEntityExpandedRecursive(scene, entity, open);
            } else {
                SetEntityExpanded(id, open);
            }
        }
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
        ImGui::PushID(NodeId(id));
        DrawSceneVisibilityControl(context, scene, entity);
        ImGui::SameLine();
        DrawScenePickabilityControl(context, scene, entity);
        ImGui::PopID();
        ImGui::SameLine();
        if (!children.empty()) {
            ImGui::SetNextItemOpen(searchActive || IsEntityExpanded(id), ImGuiCond_Always);
        }
        open = ImGui::TreeNodeEx(NodeId(id), flags, "%s", entity.GetName().c_str());
        if (!searchActive && ImGui::IsItemToggledOpen()) {
            if (ImGui::GetIO().KeyAlt) {
                SetEntityExpandedRecursive(scene, entity, open);
            } else {
                SetEntityExpanded(id, open);
            }
        }
        const ImRect rowRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        EditorTooltip::ForLastItem("Drag to reorder or parent this entity. Double-click to rename.");

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            const ImGuiIO& clickIo = ImGui::GetIO();
            if (clickIo.KeyShift) {
                context.SelectSceneRange(m_VisibleEntityRows, id, /*additive=*/clickIo.KeyCtrl);
            } else if (clickIo.KeyCtrl) {
                context.ToggleSceneEntitySelection(id);
            } else if (ShouldPreserveSelectionForPotentialDrag(selection, id)) {
                // Keep the selected set intact so a drag from this row moves the group.
            } else {
                context.SelectSceneEntity(id);
            }
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            BeginRename(id, entity.GetName());
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            const std::uint64_t value = id.Value();
            ImGui::SetDragDropPayload(kEntityDragType, &value, sizeof(value));
            const std::vector<UUID> moveTargets = ResolveDragMoveTargets(scene, selection, id);
            if (moveTargets.size() > 1) {
                ImGui::Text("%zu GameObjects", moveTargets.size());
            } else {
                ImGui::TextUnformatted(entity.GetName().c_str());
            }
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(kEntityDragType, ImGuiDragDropFlags_AcceptPeekOnly)) {
                const std::uint64_t value = *static_cast<const std::uint64_t*>(payload->Data);
                const UUID movedId(value);
                const std::vector<UUID> moveTargets = ResolveDragMoveTargets(scene, selection, movedId);
                const EntityDropPlacement placement = DropPlacementForCurrentItem();
                if (IsEntityDropValid(scene, moveTargets, entity, placement)) {
                    DrawHierarchyDropPreview(rowRect, placement);
                    if (payload->IsDelivery()) {
                        m_PendingKind = PendingKind::MoveEntity;
                        m_PendingTarget = movedId;
                        m_PendingMoveTargets = moveTargets;
                        if (placement == EntityDropPlacement::Inside) {
                            m_PendingParent = id;
                            m_PendingSiblingIndex = scene.GetChildren(entity).size();
                        } else {
                            const UUID parentId = ParentIdOf(scene, entity);
                            std::size_t siblingIndex = scene.GetSiblingIndex(entity);
                            if (placement == EntityDropPlacement::After) {
                                ++siblingIndex;
                            }
                            m_PendingParent = parentId;
                            m_PendingSiblingIndex = siblingIndex;
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
                context.SelectSceneEntity(id);
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
            const bool canCreateEmptyParent = EditorCommands::CanCreateEmptyParent(scene, selection.All());
            if (ImGui::MenuItem("Create Empty Parent", nullptr, false, canCreateEmptyParent)) {
                m_PendingKind = PendingKind::CreateEmptyParent;
            }
            EditorTooltip::ForLastItem("Wraps the selected same-level entities in a new parent.");
            if (ImGui::MenuItem("Set as Default Parent")) {
                context.SetHierarchyDefaultParent(id);
            }
            EditorTooltip::ForLastItem("Makes future Create Empty actions parent under this entity.");
            if (ImGui::MenuItem("Clear Default Parent", nullptr, false, context.HierarchyDefaultParent().IsValid())) {
                context.ClearHierarchyDefaultParent();
            }
            EditorTooltip::ForLastItem("Clears the hierarchy default parent.");
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

    if (defaultParent) {
        ImGui::PopStyleColor();
    }

    if (open) {
        for (const Entity& child : children) {
            if (visibleFilter == nullptr || visibleFilter->contains(child.GetUUID().Value())) {
                DrawEntityNode(context, scene, child, visibleFilter, searchActive);
            }
        }
        ImGui::TreePop();
    }
}

void HierarchyPanel::DropStaleExpandedIds(const Scene& scene) {
    for (auto it = m_ExpandedEntityIds.begin(); it != m_ExpandedEntityIds.end();) {
        if (scene.ContainsUUID(UUID(*it))) {
            ++it;
        } else {
            it = m_ExpandedEntityIds.erase(it);
        }
    }
}

bool HierarchyPanel::IsEntityExpanded(UUID id) const {
    return id.IsValid() && m_ExpandedEntityIds.contains(id.Value());
}

void HierarchyPanel::SetEntityExpanded(UUID id, bool expanded) {
    if (!id.IsValid()) {
        return;
    }
    if (expanded) {
        m_ExpandedEntityIds.insert(id.Value());
    } else {
        m_ExpandedEntityIds.erase(id.Value());
    }
}

void HierarchyPanel::SetEntityExpandedRecursive(Scene& scene, const Entity& entity, bool expanded) {
    if (!entity) {
        return;
    }
    SetEntityExpanded(entity.GetUUID(), expanded);
    for (const Entity& child : scene.GetChildren(entity)) {
        SetEntityExpandedRecursive(scene, child, expanded);
    }
}

void HierarchyPanel::DrawRootDropTarget(EditorContext& context, Scene& scene) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr) {
        return;
    }
    if (ImGui::BeginDragDropTargetCustom(window->InnerRect, window->ID)) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(kEntityDragType, ImGuiDragDropFlags_AcceptPeekOnly)) {
            const std::uint64_t value = *static_cast<const std::uint64_t*>(payload->Data);
            const UUID movedId(value);
            const std::vector<UUID> moveTargets = ResolveDragMoveTargets(scene, context.selection, movedId);
            if (!moveTargets.empty()) {
                ImRect rootPreview = window->InnerRect;
                rootPreview.Min.y = std::max(rootPreview.Min.y, rootPreview.Max.y - ImGui::GetFrameHeight());
                DrawHierarchyDropPreview(rootPreview, EntityDropPlacement::After);
                if (payload->IsDelivery()) {
                    m_PendingKind = PendingKind::MoveEntity;
                    m_PendingTarget = movedId;
                    m_PendingMoveTargets = moveTargets;
                    m_PendingParent = UUID(0);
                    m_PendingSiblingIndex = scene.GetRootEntities().size();
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
    std::vector<UUID> moveTargets = m_PendingMoveTargets;
    m_PendingKind = PendingKind::None;
    m_PendingTarget = UUID(0);
    m_PendingParent = UUID(0);
    m_PendingSiblingIndex = 0;
    m_PendingPrefabPath.clear();
    m_PendingMoveTargets.clear();

    switch (kind) {
    case PendingKind::None:
        return;
    case PendingKind::CreateEmpty:
        context.undoRedo.Execute(EditorCommands::CreateEntity("GameObject", context.DefaultParentFor(scene)), context);
        break;
    case PendingKind::CreateEmptyParent:
        if (EditorCommands::CanCreateEmptyParent(scene, context.selection.All())) {
            context.undoRedo.Execute(EditorCommands::CreateEmptyParent(scene, context.selection.All()), context);
        }
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
        if (moveTargets.empty() && target.IsValid()) {
            moveTargets.push_back(target);
        }
        Entity newParent;
        if (parent.IsValid()) {
            newParent = scene.FindEntityByUUID(parent);
        }
        // Cannot parent an entity to itself or to one of its own descendants. Invalid
        // parent id means scene root.
        bool validMove = !moveTargets.empty() && (!parent.IsValid() || static_cast<bool>(newParent));
        for (const UUID moveTarget : moveTargets) {
            Entity child = scene.FindEntityByUUID(moveTarget);
            if (!child || (newParent && (child == newParent || scene.IsDescendantOf(newParent, child)))) {
                validMove = false;
                break;
            }
        }
        if (validMove) {
            if (moveTargets.size() == 1) {
                const std::size_t adjustedIndex =
                    AdjustSiblingIndexForRemoval(scene, moveTargets.front(), parent, siblingIndex);
                context.undoRedo.Execute(EditorCommands::MoveEntity(scene, moveTargets.front(), parent, adjustedIndex),
                                         context);
            } else {
                context.undoRedo.Execute(EditorCommands::MoveEntities(scene, std::move(moveTargets), parent, siblingIndex),
                                         context);
            }
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
