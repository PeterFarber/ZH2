#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunHierarchyPanelContractTests() {
    HockeyTest::BeginSuite("HierarchyPanelContractTests");

    const std::string header = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/HierarchyPanel.hpp");
    const std::string source = ReadProjectFile("libs/editor/src/Panels/HierarchyPanel.cpp");
    const std::string selectionHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/Selection.hpp");
    const std::string commandsHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorCommands.hpp");
    const std::string commandsSource = ReadProjectFile("libs/editor/src/EditorCommands.cpp");
    const std::string rendererHeader = ReadProjectFile("libs/renderer/include/Hockey/Renderer/Renderer.hpp");
    const std::string rendererSource = ReadProjectFile("libs/renderer/src/Renderer.cpp");
    const std::string menuSource = ReadProjectFile("libs/editor/src/MainMenuBar.cpp");
    const std::string appSource = ReadProjectFile("libs/editor/src/EditorApp.cpp");
    const std::string sceneViewportSource = ReadProjectFile("libs/editor/src/Panels/SceneViewportPanel.cpp");
    const std::string selectionOutlineHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/Gizmos/SelectionOutline.hpp");
    const std::string cameraLightHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/Gizmos/CameraLightGizmo.hpp");
    const std::string physicsGizmoHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/Gizmos/PhysicsGizmo.hpp");

    HK_CHECK_MSG(Contains(header, "MoveEntity"), "Hierarchy panel has a deferred move operation");
    HK_CHECK_MSG(Contains(header, "m_PendingSiblingIndex"), "Hierarchy panel stores the requested sibling index");
    HK_CHECK_MSG(Contains(source, "EditorCommands::MoveEntity"),
                 "Hierarchy drag/drop uses the undoable move command");
    HK_CHECK_MSG(Contains(source, "GetSiblingIndex"), "Hierarchy drag/drop computes sibling insertion indexes");
    HK_CHECK_MSG(Contains(source, "GetItemRectMin") && Contains(source, "GetItemRectMax"),
                 "Hierarchy drag/drop splits rows into before/inside/after drop bands");
    HK_CHECK_MSG(Contains(source, "DrawRootDropTarget(EditorContext& context, Scene& scene)"),
                 "Hierarchy root drop target can move entities to root order");
    HK_CHECK_MSG(Contains(source, "DrawHierarchyDropPreview"),
                 "Hierarchy drag/drop draws a custom visual preview");
    HK_CHECK_MSG(Contains(source, "ImGuiDragDropFlags_AcceptPeekOnly"),
                 "Hierarchy entity drops preview before delivery without ImGui's default target rectangle");
    HK_CHECK_MSG(Contains(source, "payload->IsDelivery()"),
                 "Hierarchy drag/drop only mutates the scene when the payload is delivered");
    HK_CHECK_MSG(Contains(source, "AddRectFilled") && Contains(source, "AddLine"),
                 "Hierarchy preview distinguishes inside-parent drops from sibling insertion lines");
    HK_CHECK_MSG(Contains(header, "m_SearchBuffer"), "Hierarchy panel stores local search text");
    HK_CHECK_MSG(Contains(header, "m_FocusSearchNextFrame"), "Hierarchy panel can focus search from shortcut");
    HK_CHECK_MSG(Contains(source, "DrawSearchToolbar"), "Hierarchy panel draws a search toolbar");
    HK_CHECK_MSG(Contains(source, "InputTextWithHint") && Contains(source, "Search GameObjects"),
                 "Hierarchy panel exposes a search input");
    HK_CHECK_MSG(Contains(source, "Clear Search"), "Hierarchy panel exposes a clear-search button");
    HK_CHECK_MSG(Contains(source, "BuildHierarchyFilter"), "Hierarchy panel builds an ancestor-preserving search filter");
    HK_CHECK_MSG(Contains(source, "HierarchyNameMatchesSearch"), "Hierarchy filter matches entity names case-insensitively");
    HK_CHECK_MSG(Contains(source, "ImGuiKey_F") && Contains(source, "KeyCtrl"),
                 "Hierarchy panel supports Ctrl+F search focus");
    HK_CHECK_MSG(Contains(header, "m_ExpandedEntityIds"), "Hierarchy panel owns persistent expansion state");
    HK_CHECK_MSG(Contains(source, "DropStaleExpandedIds"), "Hierarchy panel prunes expansion ids for deleted entities");
    HK_CHECK_MSG(Contains(source, "SetEntityExpandedRecursive"),
                 "Hierarchy panel has a recursive expand/collapse helper");
    HK_CHECK_MSG(Contains(source, "IsEntityExpanded"), "Hierarchy panel reads explicit foldout state");
    HK_CHECK_MSG(Contains(source, "ImGui::GetIO().KeyAlt") && Contains(source, "ImGui::IsItemToggledOpen"),
                 "Hierarchy foldout Alt-click toggles the full subtree");
    HK_CHECK_MSG(Contains(source, "!searchActive") && Contains(source, "SetNextItemOpen"),
                 "Hierarchy search can force open draw state without changing saved expansion");
    HK_CHECK_MSG(Contains(source, "Create Empty Parent"), "Hierarchy context menu exposes create-empty-parent");
    HK_CHECK_MSG(Contains(source, "Set as Default Parent"), "Hierarchy context menu can set a default parent");
    HK_CHECK_MSG(Contains(source, "Clear Default Parent"), "Hierarchy context menu can clear the default parent");
    HK_CHECK_MSG(Contains(source, "DefaultParentFor"), "Hierarchy create-empty uses the valid default parent");
    HK_CHECK_MSG(Contains(source, "PushStyleColor") && Contains(source, "HierarchyDefaultParent"),
                 "Hierarchy marks the default-parent row distinctly");
    HK_CHECK_MSG(Contains(menuSource, "Create Empty Parent") && Contains(menuSource, "Ctrl+Shift+G"),
                 "GameObject menu exposes create-empty-parent shortcut text");
    HK_CHECK_MSG(Contains(menuSource, "DefaultParentFor"), "GameObject menu creates empty objects under default parent");
    HK_CHECK_MSG(Contains(appSource, "ImGuiKey_G") && Contains(appSource, "CreateEmptyParent"),
                 "Editor shortcuts support Ctrl+Shift+G for create-empty-parent");
    HK_CHECK_MSG(Contains(appSource, "ImGuiKey_N") && Contains(appSource, "DefaultParentFor"),
                 "Editor shortcuts support Ctrl+Shift+N for create empty under default parent");
    HK_CHECK_MSG(Contains(source, "DrawSceneVisibilityControl") && Contains(source, "DrawScenePickabilityControl"),
                 "Hierarchy rows draw visibility and pickability controls before names");
    HK_CHECK_MSG(Contains(source, "VisibilityMixed") && Contains(source, "PickabilityMixed"),
                 "Hierarchy rows expose mixed parent/child state markers");
    HK_CHECK_MSG(Contains(source, "ImGuiKey_H") && Contains(source, "ToggleSelectedVisibility"),
                 "Hierarchy supports H shortcut for selected scene visibility");
    HK_CHECK_MSG(Contains(source, "ImGuiKey_L") && Contains(source, "ToggleSelectedPickability"),
                 "Hierarchy supports L shortcut for selected scene pickability");
    HK_CHECK_MSG(Contains(source, "ImGui::GetIO().KeyAlt") && Contains(source, "SetSceneHidden") &&
                     Contains(source, "SetSceneUnpickable"),
                 "Hierarchy Alt-click toggles only the clicked visibility/pickability entity");
    HK_CHECK_MSG(Contains(sceneViewportSource, "IsScenePickable") && Contains(sceneViewportSource, "IsSceneHidden"),
                 "Scene viewport picking skips editor-hidden or unpickable entities");
    HK_CHECK_MSG(Contains(rendererHeader, "SceneRenderFilter") &&
                     Contains(rendererHeader, "RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target,"),
                 "Renderer supports editor-supplied scene visibility filters");
    HK_CHECK_MSG(Contains(rendererSource, "AllowsEntity") && Contains(rendererSource, "filter->Allows"),
                 "Renderer skips filtered entities while collecting lights and draw items");
    HK_CHECK_MSG(Contains(sceneViewportSource, "MakeSceneRenderFilter") &&
                     Contains(sceneViewportSource, "RenderSceneToTarget(*context.activeScene, sceneCamera, m_Target, &renderFilter)"),
                 "Scene viewport passes editor-hidden state into rendering");
    HK_CHECK_MSG(Contains(selectionOutlineHeader, "EditorContext") &&
                     Contains(sceneViewportSource, "SelectionOutline::Submit(debug, context)"),
                 "Selection outlines do not draw for hidden entities");
    HK_CHECK_MSG(Contains(cameraLightHeader, "EditorContext") &&
                     Contains(sceneViewportSource, "CameraLightGizmo::Submit(debug, context, viewportAspect)"),
                 "Camera/light gizmos do not draw for hidden entities");
    HK_CHECK_MSG(Contains(physicsGizmoHeader, "EditorContext") &&
                     Contains(sceneViewportSource, "PhysicsGizmo::Submit(debug, context)"),
                 "Physics debug overlays do not draw for hidden entities");
    HK_CHECK_MSG(Contains(sceneViewportSource, "CanManipulateSelection") && Contains(sceneViewportSource, "IsScenePickable"),
                 "Scene viewport disables transform manipulation for hidden or locked selections");
    HK_CHECK_MSG(Contains(selectionHeader, "SelectRange") && Contains(selectionHeader, "RangeAnchor"),
                 "Selection exposes a pure visible-row range selection helper");
    HK_CHECK_MSG(Contains(header, "m_VisibleEntityRows"),
                 "Hierarchy panel tracks visible row order for Shift range selection");
    HK_CHECK_MSG(Contains(source, "KeyShift") && Contains(source, "SelectSceneRange"),
                 "Hierarchy row clicks support Shift range selection");
    HK_CHECK_MSG(Contains(source, "SelectSceneEntity") && Contains(source, "ToggleSceneEntitySelection") &&
                     Contains(source, "SelectSceneRange"),
                 "Hierarchy row clicks route through lock-aware editor-context selection helpers");
    HK_CHECK_MSG(Contains(source, "ResolveDragMoveTargets"),
                 "Hierarchy drag/drop resolves selected top-level rows when dragging a selected entity");
    HK_CHECK_MSG(Contains(source, "ShouldPreserveSelectionForPotentialDrag") &&
                     Contains(source, "selection.Count() > 1") && Contains(source, "selection.IsSelected(id)"),
                 "Hierarchy click handling preserves an existing multi-selection so a drag can move it as a group");
    HK_CHECK_MSG(Contains(commandsHeader, "MoveEntities") && Contains(commandsSource, "MoveEntitiesCommand"),
                 "Hierarchy selected-group drag/drop has one undoable multi-entity move command");
    HK_CHECK_MSG(Contains(source, "EditorCommands::MoveEntities"),
                 "Hierarchy drag/drop uses the undoable multi-entity move command for selected groups");
}
