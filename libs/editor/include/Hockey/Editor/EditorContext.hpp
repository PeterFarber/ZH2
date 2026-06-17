#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

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

// Parameters used to wire the editor to the host application's subsystems.
struct EditorContextCreateInfo {
    Window* window = nullptr;
    Renderer* renderer = nullptr;
    Scene* scene = nullptr;
    Config* config = nullptr;
    AssetManager* assetManager = nullptr;
};

// Frame of reference for transform gizmos / the toolbar's space toggle.
enum class TransformSpace {
    World,
    Local,
};

// Physics preview + visualization state (runtime; not persisted). Drives the
// collider/trigger overlay in the scene viewport and the Physics panel's
// preview controls. The preview simulation itself is owned by EditorApp.
struct PhysicsViewState {
    bool previewEnabled = false; // simulate physics in the Edit-mode viewport
    bool previewRunning = false; // play/pause within preview
    bool showColliders = true;
    bool showTriggers = true;
    bool showBodyCenters = false;
    bool showContacts = false;

    // Contact points captured during the last preview step (world space), filled
    // by EditorApp when showContacts is on. Drawn as crosses by the PhysicsGizmo.
    std::vector<glm::vec3> contactPoints;
};

struct GameplayPreviewState {
    bool previewEnabled = false;
    bool previewRunning = false;
    bool showDebug = false;
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
    bool simulateMode = false;

    // Physics collider/trigger overlay + preview toggles (see PhysicsViewState).
    PhysicsViewState physicsView;
    GameplayPreviewState gameplayView;

    // Owned by EditorApp; null in headless tests without physics. The Physics
    // panel drives it (start/stop/step/reset) and EditorApp ticks it each frame.
    EditorPhysicsPreview* physicsPreview = nullptr;
    EditorGameplayPreview* gameplayPreview = nullptr;

    // Set by panels (e.g. double-clicking a scene in the Project panel) to ask
    // the host to open a scene file. EditorApp drains this each frame, running
    // the unsaved-changes prompt, then clears it.
    std::filesystem::path requestedOpenScene;

    // Size of the scene viewport's offscreen target in pixels, refreshed each
    // frame by the SceneViewportPanel. Consumed by the Stats panel. Zero until
    // the scene viewport has been drawn at least once.
    std::uint32_t viewportWidth = 0;
    std::uint32_t viewportHeight = 0;

    // Active gizmo space, toggled from the toolbar; consumed by the transform
    // gizmo in a later step.
    TransformSpace transformSpace = TransformSpace::World;

    // Marks the active scene as modified (drives the save prompt / dirty
    // indicator). Calling with false clears the flag (e.g. after a save).
    void MarkDirty(bool dirty = true) {
        sceneDirty = dirty;
    }

    bool HasActiveScenePath() const {
        return !activeScenePath.empty();
    }
};

} // namespace Hockey
