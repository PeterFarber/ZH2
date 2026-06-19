#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
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

    // Owned by EditorApp; null in headless tests without physics. Gameplay
    // playtest drives it and EditorApp ticks it each frame.
    EditorPhysicsPreview* physicsPreview = nullptr;
    EditorGameplayPreview* gameplayPreview = nullptr;

    // Set by panels (e.g. double-clicking a scene in the Project panel) to ask
    // the host to open a scene file. EditorApp drains this each frame, running
    // the unsaved-changes prompt, then clears it.
    std::filesystem::path requestedOpenScene;

    // Set by commands that need a docked panel to become active on the next UI
    // pass. PanelManager consumes and clears this after applying focus.
    std::string requestedPanelFocus;

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
};

} // namespace Hockey
