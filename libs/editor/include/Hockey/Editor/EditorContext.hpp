#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Clipboard.hpp"
#include "Hockey/Editor/EditorCamera.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/Gizmos/GizmoOperation.hpp"
#include "Hockey/Editor/PanelManager.hpp"
#include "Hockey/Editor/Selection.hpp"
#include "Hockey/Editor/Tools/ToolManager.hpp"
#include "Hockey/Editor/UndoRedo.hpp"

namespace Hockey {

class Window;
class Renderer;
class Scene;
class Config;
class ImGuiRendererBridge;
class AssetManager;
class EditorPhysicsPreview;
class EditorGameplayPreview;
class EditorClientPreview;

// Parameters used to wire the editor to the host application's subsystems.
struct EditorContextCreateInfo {
    Window* window = nullptr;
    Renderer* renderer = nullptr;
    Scene* scene = nullptr;
    Config* config = nullptr;
    std::filesystem::path configPath;
    AssetManager* assetManager = nullptr;
    bool captureSceneViewport = false;
    bool captureGameViewport = false;
    std::string captureViewportPrefix = "editor";
    std::uint32_t captureViewportWidth = 1920;
    std::uint32_t captureViewportHeight = 1080;
};

// Frame of reference for transform gizmos / the toolbar's space toggle.
enum class TransformSpace {
    World,
    Local,
};

// Physics visualization state (runtime; not persisted). The preview simulation
// itself is an internal playtest backend owned by EditorApp.
struct PhysicsViewState {
    bool previewEnabled = false; // internal backend state, surfaced for status only
    bool previewRunning = false; // internal backend state, surfaced for status only
    bool showColliders = true;
    bool showTriggers = true;
    bool showBodyCenters = false;
    bool showContacts = false;
    bool visualizationOptionsOpen = false;

    // Contact points captured during the last preview step (world space), filled
    // by EditorApp when showContacts is on. Drawn as crosses by the PhysicsGizmo.
    std::vector<glm::vec3> contactPoints;
};

struct GameplayPreviewState {
    bool previewEnabled = false;
    bool previewRunning = false;
    bool showDebug = false;
    bool gameInputActive = false;
};

struct ClientPreviewState {
    bool previewEnabled = false;
    bool previewRunning = false;
    bool gameInputActive = false;
    std::uint32_t viewportWidth = 0;
    std::uint32_t viewportHeight = 0;
};

// Central editor state shared by every panel, tool, command and gizmo. The
// context owns editor-side state only; the Scene still owns scene data and the
// Renderer still owns GPU resources. Later Phase 4 steps add the EditorCamera
// and ToolManager members.
class EditorContext {
public:
    Window* window = nullptr;
    Renderer* renderer = nullptr;
    Scene* activeScene = nullptr;
    Config* config = nullptr;
    std::filesystem::path configPath;

    // Borrowed from the host application; owns the content pipeline (database,
    // import/cook, runtime loading, hot-reload). Null in headless tests.
    AssetManager* assetManager = nullptr;

    // Owned by the ImGuiLayer; the scene/game viewport panels use it to bind
    // offscreen render targets as ImGui textures. Null in headless tests.
    ImGuiRendererBridge* imguiBridge = nullptr;

    EditorSettings settings;

    Selection selection;
    EditorCamera editorCamera;
    PanelManager panelManager;
    ToolManager toolManager;
    UndoRedoStack undoRedo;
    Clipboard clipboard;

    // Active viewport gizmo mode, driven by the transform tools and consumed by
    // the SceneViewportPanel's gizmo.
    GizmoOperation gizmoOperation = GizmoOperation::Translate;

    std::filesystem::path activeScenePath;
    bool sceneDirty = false;
    bool playMode = false;

    // Physics collider/trigger overlay + internal playtest backend state.
    PhysicsViewState physicsView;
    GameplayPreviewState gameplayView;
    ClientPreviewState clientPreview;

    // Owned by EditorApp; null in headless tests without physics. Gameplay
    // playtest drives it and EditorApp ticks it each frame.
    EditorPhysicsPreview* physicsPreview = nullptr;
    EditorGameplayPreview* gameplayPreview = nullptr;
    EditorClientPreview* clientPreviewHost = nullptr;

    // Set by panels (e.g. double-clicking a scene in the Project panel) to ask
    // the host to open a scene file. EditorApp drains this each frame, running
    // the unsaved-changes prompt, then clears it.
    std::filesystem::path requestedOpenScene;

    // Set by commands that need a docked panel to become active on the next UI
    // pass. PanelManager consumes and clears this after applying focus.
    std::string requestedPanelFocus;
    std::filesystem::path clientFlowAssetPath;
    bool requestClientPreviewStart = false;

    bool captureSceneViewport = false;
    bool captureGameViewport = false;
    std::string captureViewportPrefix = "editor";
    std::uint32_t captureViewportWidth = 1920;
    std::uint32_t captureViewportHeight = 1080;

    // Size of the scene viewport's offscreen target in pixels, refreshed each
    // frame by the SceneViewportPanel. Consumed by the Stats panel. Zero until
    // the scene viewport has been drawn at least once.
    std::uint32_t viewportWidth = 0;
    std::uint32_t viewportHeight = 0;

    // Active gizmo space, toggled from the toolbar; consumed by the transform
    // gizmo in a later step.
    TransformSpace transformSpace = TransformSpace::World;

    void SetHierarchyDefaultParent(UUID id) {
        m_HierarchyDefaultParentId = id;
    }

    void ClearHierarchyDefaultParent() {
        m_HierarchyDefaultParentId = UUID(0);
    }

    UUID HierarchyDefaultParent() const {
        return m_HierarchyDefaultParentId;
    }

    UUID DefaultParentFor(const Scene& scene) const {
        if (m_HierarchyDefaultParentId.IsValid() && scene.ContainsUUID(m_HierarchyDefaultParentId)) {
            return m_HierarchyDefaultParentId;
        }
        return UUID(0);
    }

    bool IsSceneHidden(UUID id) const {
        return id.IsValid() && m_SceneHiddenIds.contains(id.Value());
    }

    bool IsScenePickable(UUID id) const {
        return !id.IsValid() || !m_SceneUnpickableIds.contains(id.Value());
    }

    bool CanSelectSceneEntity(UUID id) const {
        if (!id.IsValid() || !IsScenePickable(id)) {
            return false;
        }
        return activeScene == nullptr || activeScene->ContainsUUID(id);
    }

    bool SelectSceneEntity(UUID id) {
        if (!id.IsValid()) {
            selection.Clear();
            return true;
        }
        if (!CanSelectSceneEntity(id)) {
            return false;
        }
        selection.Select(id);
        SyncAssetSelectionWithEntitySelection();
        return true;
    }

    bool ToggleSceneEntitySelection(UUID id) {
        if (!id.IsValid()) {
            return false;
        }
        if (selection.IsSelected(id)) {
            selection.Remove(id);
            return true;
        }
        if (!CanSelectSceneEntity(id)) {
            return false;
        }
        selection.Add(id);
        SyncAssetSelectionWithEntitySelection();
        return true;
    }

    bool SelectSceneRange(const std::vector<UUID>& orderedVisibleIds, UUID targetId, bool additive) {
        if (!CanSelectSceneEntity(targetId)) {
            return false;
        }

        std::vector<UUID> pickableRows;
        pickableRows.reserve(orderedVisibleIds.size());
        for (const UUID id : orderedVisibleIds) {
            if (CanSelectSceneEntity(id)) {
                pickableRows.push_back(id);
            }
        }

        selection.SelectRange(pickableRows, targetId, additive);
        SyncAssetSelectionWithEntitySelection();
        return true;
    }

    void SelectAllSceneEntities(Scene& scene) {
        selection.Clear();
        for (Entity entity : scene.GetAllEntities()) {
            const UUID id = entity.GetUUID();
            if (id.IsValid() && IsScenePickable(id)) {
                selection.Add(id);
            }
        }
        SyncAssetSelectionWithEntitySelection();
    }

    void SetSceneHidden(Scene& scene, UUID id, bool hidden, bool includeDescendants = true) {
        if (!id.IsValid() || !scene.ContainsUUID(id)) {
            return;
        }
        if (hidden) {
            m_SceneHiddenIds.insert(id.Value());
        } else {
            m_SceneHiddenIds.erase(id.Value());
        }
        if (!includeDescendants) {
            return;
        }
        if (Entity entity = scene.FindEntityByUUID(id)) {
            for (const Entity& child : scene.GetChildren(entity)) {
                SetSceneHidden(scene, child.GetUUID(), hidden, true);
            }
        }
    }

    void SetSceneUnpickable(Scene& scene, UUID id, bool unpickable, bool includeDescendants = true) {
        if (!id.IsValid() || !scene.ContainsUUID(id)) {
            return;
        }
        if (unpickable) {
            m_SceneUnpickableIds.insert(id.Value());
            selection.Remove(id);
        } else {
            m_SceneUnpickableIds.erase(id.Value());
        }
        if (!includeDescendants) {
            return;
        }
        if (Entity entity = scene.FindEntityByUUID(id)) {
            for (const Entity& child : scene.GetChildren(entity)) {
                SetSceneUnpickable(scene, child.GetUUID(), unpickable, true);
            }
        }
    }

    void ToggleSelectedVisibility(Scene& scene) {
        bool hide = false;
        for (const UUID id : selection.All()) {
            if (scene.ContainsUUID(id) && !IsSceneHidden(id)) {
                hide = true;
                break;
            }
        }
        for (const UUID id : selection.All()) {
            SetSceneHidden(scene, id, hide, true);
        }
    }

    void ToggleSelectedPickability(Scene& scene) {
        bool lock = false;
        const std::vector<UUID> selectedIds = selection.All();
        for (const UUID id : selectedIds) {
            if (scene.ContainsUUID(id) && IsScenePickable(id)) {
                lock = true;
                break;
            }
        }
        for (const UUID id : selectedIds) {
            SetSceneUnpickable(scene, id, lock, true);
        }
    }

    void PruneHierarchyEditorState(const Scene& scene) {
        for (auto it = m_SceneHiddenIds.begin(); it != m_SceneHiddenIds.end();) {
            if (scene.ContainsUUID(UUID(*it))) {
                ++it;
            } else {
                it = m_SceneHiddenIds.erase(it);
            }
        }
        for (auto it = m_SceneUnpickableIds.begin(); it != m_SceneUnpickableIds.end();) {
            if (scene.ContainsUUID(UUID(*it))) {
                ++it;
            } else {
                it = m_SceneUnpickableIds.erase(it);
            }
        }
    }

    // Cooked asset currently selected by Project-style panels. Entity selection
    // remains the primary scene selection model; selecting an asset clears it so
    // Inspector can switch cleanly between entity and asset editing.
    void SelectAsset(AssetID id) {
        m_SelectedAsset = id;
        if (id.IsValid()) {
            selection.Clear();
        }
    }

    void ClearAssetSelection() {
        m_SelectedAsset = {};
    }

    AssetID SelectedAsset() const {
        return m_SelectedAsset;
    }

    void SyncAssetSelectionWithEntitySelection() {
        if (m_SelectedAsset.IsValid() && selection.Primary().IsValid()) {
            m_SelectedAsset = {};
        }
    }

    // Marks the active scene as modified (drives the save prompt / dirty
    // indicator). Calling with false clears the flag (e.g. after a save).
    void MarkDirty(bool dirty = true) {
        if (playMode && dirty) {
            return;
        }
        sceneDirty = dirty;
    }

    bool HasActiveScenePath() const {
        return !activeScenePath.empty();
    }

private:
    AssetID m_SelectedAsset;
    UUID m_HierarchyDefaultParentId{0};
    std::unordered_set<std::uint64_t> m_SceneHiddenIds;
    std::unordered_set<std::uint64_t> m_SceneUnpickableIds;
};

} // namespace Hockey
