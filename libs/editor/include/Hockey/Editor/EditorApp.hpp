#pragma once

#include <cstdint>
#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/ImGui/ImGuiLayer.hpp"
#include "Hockey/Editor/MainMenuBar.hpp"
#include "Hockey/Editor/SceneWorkflow.hpp"
#include "Hockey/Editor/Toolbar.hpp"

namespace Hockey {

// Returns the human-readable version string of the editor library.
const char* EditorVersionString();

// The central editor application object. It owns the editor-side systems (the
// ImGui layer, dockspace, menu bar, toolbar and, in later steps, the
// panel/tool/undo managers) and drives the per-frame UI + render loop. The host
// shell (apps/map_editor) owns the Window, Renderer, Scene and Config and passes
// them in through EditorContextCreateInfo.
class EditorApp {
public:
    EditorApp() = default;
    ~EditorApp();

    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;

    Status Init(const EditorContextCreateInfo& info);
    void Shutdown();

    // Forwards a raw SDL_Event (as void*) to the ImGui layer.
    void ProcessRawSDLEvent(const void* sdlEvent);

    // Per-frame loop, called in order by the host shell.
    void BeginFrame(float deltaTime);
    void Update(float deltaTime);
    void Render();
    void EndFrame();

    bool WantsQuit() const {
        return m_WantsQuit;
    }

    EditorContext& Context() {
        return m_Context;
    }

    // ----- Actions invoked by the menu bar / toolbar -----
    // Requests application exit. Prompts to save when the scene is dirty.
    void RequestQuit();
    void ResetLayout() {
        m_Dockspace.RequestReset();
    }

    // ----- Scene lifecycle (menu bar / toolbar / shortcuts) -----
    // New/Open prompt to save when the scene is dirty before discarding it.
    void NewScene();
    void OpenScene();                                  // native open dialog
    void OpenScene(const std::filesystem::path& path); // recent scenes / project panel
    bool SaveScene();                                  // save to active path (else Save As)
    bool SaveSceneAs();                                // native save dialog
    void OpenAutosaveFolder();

    // Imports an external asset (model/texture/material/shader/...) selected via a
    // native open dialog: copies it into the matching data/raw/<type> folder and
    // cooks it (plus any sub-assets / embedded textures) so it is usable at once.
    void ImportAsset();
    // Runs the scene validator and reports the result (Console panel later; logs
    // for now). Returns the number of issues found.
    int ValidateActiveScene();
    // Toggles play / simulate scene-lifecycle modes (no gameplay in Phase 4).
    void TogglePlayMode();
    void ToggleSimulateMode();

    // ----- Undo / clipboard actions (Edit menu + keyboard shortcuts) -----
    void Undo();
    void Redo();
    void CopySelection();
    void CutSelection();
    void PasteFromClipboard(bool asChildOfSelection);
    void DuplicateSelection();
    void DeleteSelection();
    void SelectAllEntities();
    void DeselectAll();

    // Opens (and focuses) the Preferences panel from the Edit menu.
    void OpenPreferences();

private:
    // A scene-switching action that must be deferred until the unsaved-changes
    // prompt is answered (or run immediately when the scene is not dirty).
    enum class PendingAction {
        None,
        NewScene,
        OpenDialog,
        OpenPath,
        Quit,
    };

    void RegisterPanels();
    void RegisterTools();
    void BuildDockspaceUI();
    void ProcessShortcuts();
    // Polls the asset manager for changed raw files (when hot reload is enabled),
    // recooks dirty assets and invalidates the renderer's cached GPU resources.
    void PollAssetHotReload();

    // Either runs 'action' now (scene clean) or arms the unsaved-changes prompt.
    void RequestSceneAction(PendingAction action, const std::filesystem::path& path = {});
    void PerformPendingAction();
    void DrawUnsavedPrompt();
    bool DoSaveScene();   // save to active path, falling back to Save As
    bool DoSaveSceneAs(); // native dialog + serialize
    void DoOpenScene(const std::filesystem::path& path);
    // Copies 'source' into data/raw/<type> and runs import + cook on it.
    void DoImportAsset(const std::filesystem::path& source);
    std::filesystem::path DefaultSceneDirectory() const;

    EditorContext m_Context;
    ImGuiLayer m_ImGuiLayer;
    Dockspace m_Dockspace;
    MainMenuBar m_MenuBar;
    Toolbar m_Toolbar;
    SceneWorkflow m_SceneWorkflow;
    EditorPhysicsPreview m_PhysicsPreview;

    PendingAction m_PendingAction = PendingAction::None;
    std::filesystem::path m_PendingScenePath;
    bool m_OpenUnsavedPopup = false;

    bool m_Initialized = false;
    bool m_WantsQuit = false;
    std::uint32_t m_LastWidth = 0;
    std::uint32_t m_LastHeight = 0;
};

} // namespace Hockey
