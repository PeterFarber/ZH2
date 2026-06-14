#include "Hockey/Editor/EditorApp.hpp"

#include <string>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetEvent.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/FileDialog.hpp"
#include "Hockey/Editor/Logging/EditorConsoleSink.hpp"
#include "Hockey/Editor/Panels/ConsolePanel.hpp"
#include "Hockey/Editor/Panels/GameViewportPanel.hpp"
#include "Hockey/Editor/Panels/HierarchyPanel.hpp"
#include "Hockey/Editor/Panels/InspectorPanel.hpp"
#include "Hockey/Editor/Panels/PhysicsPanel.hpp"
#include "Hockey/Editor/Panels/PrefabPanel.hpp"
#include "Hockey/Editor/Panels/ProjectPanel.hpp"
#include "Hockey/Editor/Panels/PropertiesPanel.hpp"
#include "Hockey/Editor/Panels/SceneValidationPanel.hpp"
#include "Hockey/Editor/Panels/SceneViewportPanel.hpp"
#include "Hockey/Editor/Panels/StatsPanel.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

const char* EditorVersionString() {
    return "HockeyEditor 0.4.0";
}

EditorApp::~EditorApp() {
    Shutdown();
}

Status EditorApp::Init(const EditorContextCreateInfo& info) {
    if (m_Initialized) {
        return Status::Ok();
    }
    if (info.window == nullptr || info.renderer == nullptr) {
        return Status::Fail("EditorApp::Init requires a window and renderer");
    }

    m_Context.window = info.window;
    m_Context.renderer = info.renderer;
    m_Context.activeScene = info.scene;
    m_Context.config = info.config;
    m_Context.assetManager = info.assetManager;
    m_Context.settings.Load(EditorSettings::DefaultPath());

    // Mirror engine log output into the Console panel.
    InstallEditorConsoleSink();

    if (Status s = m_ImGuiLayer.Init(*info.window, *info.renderer); !s) {
        return s;
    }
    m_Context.imguiBridge = &m_ImGuiLayer.Bridge();

    // The component metadata registry drives the inspector's add/remove menus
    // and field editing. Idempotent, so safe to (re)initialize here.
    ComponentRegistry::Get().RegisterPhase2Components();

    // Physics components contribute their own inspector metadata, YAML
    // serialization and scene-validation checks. Must run after the Phase 2
    // registration (which clears the registry first). Materials back the
    // inspector dropdown and validation.
    RegisterPhysicsComponents();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    // Wire the non-destructive physics preview (Edit mode never auto-simulates).
    PhysicsSettings physicsSettings;
    if (m_Context.config != nullptr) {
        LoadPhysicsSettings(*m_Context.config, physicsSettings);
    }
    m_PhysicsPreview.Configure(physicsSettings);
    m_Context.physicsPreview = &m_PhysicsPreview;

    RegisterPanels();
    RegisterTools();

    m_LastWidth = info.window->Width();
    m_LastHeight = info.window->Height();
    m_Initialized = true;
    HK_EDITOR_INFO("EditorApp initialized ({})", EditorVersionString());
    return Status::Ok();
}

void EditorApp::RegisterPanels() {
    PanelManager& panels = m_Context.panelManager;
    panels.AddPanel<HierarchyPanel>();
    panels.AddPanel<InspectorPanel>();
    panels.AddPanel<SceneViewportPanel>();
    panels.AddPanel<GameViewportPanel>();
    panels.AddPanel<ProjectPanel>();
    panels.AddPanel<ConsolePanel>();
    panels.AddPanel<PropertiesPanel>();
    panels.AddPanel<StatsPanel>();
    panels.AddPanel<SceneValidationPanel>();
    panels.AddPanel<PrefabPanel>();
    panels.AddPanel<PhysicsPanel>();
}

void EditorApp::RegisterTools() {
    RegisterEditorTools(m_Context.toolManager);
    // Default to the Move tool so the gizmo starts in translate mode.
    m_Context.toolManager.Activate("Move", m_Context);
}

void EditorApp::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    // Tear down any active physics preview, restoring the authored transforms.
    if (m_Context.activeScene != nullptr) {
        m_PhysicsPreview.Stop(*m_Context.activeScene);
    }
    m_Context.physicsPreview = nullptr;
    m_Context.settings.Save(EditorSettings::DefaultPath());
    m_ImGuiLayer.Shutdown();
    m_Initialized = false;
}

void EditorApp::ProcessRawSDLEvent(const void* sdlEvent) {
    m_ImGuiLayer.ProcessEvent(sdlEvent);
}

void EditorApp::BeginFrame(float /*deltaTime*/) {
    if (!m_Initialized) {
        return;
    }
    // Resize the swapchain before a frame is in flight.
    if (m_Context.window != nullptr && m_Context.renderer != nullptr) {
        const std::uint32_t width = m_Context.window->Width();
        const std::uint32_t height = m_Context.window->Height();
        if (width != m_LastWidth || height != m_LastHeight) {
            m_Context.renderer->Resize(width, height);
            m_ImGuiLayer.Bridge().InvalidateViewportTextures();
            m_LastWidth = width;
            m_LastHeight = height;
        }
    }
    m_ImGuiLayer.BeginFrame();
}

void EditorApp::Update(float deltaTime) {
    if (!m_Initialized) {
        return;
    }
    ProcessShortcuts();

    // Drain panel-requested scene opens (e.g. double-click in the Project panel)
    // through the same prompt-aware path as the menus.
    if (!m_Context.requestedOpenScene.empty()) {
        const std::filesystem::path requested = m_Context.requestedOpenScene;
        m_Context.requestedOpenScene.clear();
        OpenScene(requested);
    }

    PollAssetHotReload();

    m_Context.toolManager.Update(m_Context, deltaTime);
    m_SceneWorkflow.TickAutosave(m_Context, deltaTime);

    // Advance the physics preview (no-op unless the user enabled + is playing).
    if (m_Context.activeScene != nullptr) {
        m_PhysicsPreview.Update(*m_Context.activeScene, deltaTime);
        m_Context.physicsView.previewRunning = m_PhysicsPreview.IsRunning();
        m_Context.physicsView.contactPoints.clear();
        if (m_Context.physicsView.showContacts && m_PhysicsPreview.IsRunning()) {
            m_Context.physicsView.contactPoints = m_PhysicsPreview.ContactPoints();
        }
    }

    m_Context.panelManager.OnUpdate(m_Context, deltaTime);
}

void EditorApp::PollAssetHotReload() {
    AssetManager* assets = m_Context.assetManager;
    if (assets == nullptr || !m_Context.settings.assetsHotReload) {
        return;
    }

    // Detect changed raw files, re-import and (optionally) recook them. The
    // manager marks dependents dirty so a changed texture recooks its materials.
    const int changed =
        assets->PollHotReload(m_Context.settings.assetsAutoImport, m_Context.settings.assetsAutoCookDirty);
    if (changed > 0 && m_Context.settings.assetsAutoCookDirty) {
        assets->CookAllDirty();
        assets->SaveDatabase();
    }

    // Drop the renderer's cached GPU resources for every asset that changed so
    // the next frame re-uploads them from the freshly cooked data.
    const std::vector<AssetEvent>& events = assets->PollEvents();
    if (!events.empty() && m_Context.renderer != nullptr) {
        for (const AssetEvent& event : events) {
            m_Context.renderer->InvalidateAsset(event.id.Value());
        }
    }
}

void EditorApp::Render() {
    if (!m_Initialized) {
        return;
    }
    if (m_Context.renderer != nullptr) {
        m_Context.renderer->BeginFrame();
    }
    // Scene-to-target rendering for viewport panels is added in a later step.
    BuildDockspaceUI();
}

void EditorApp::EndFrame() {
    if (!m_Initialized) {
        return;
    }
    m_ImGuiLayer.EndFrame(); // ImGui::Render + record overlay into the active frame
    if (m_Context.renderer != nullptr) {
        m_Context.renderer->EndFrame();
    }
}

void EditorApp::BuildDockspaceUI() {
    m_Dockspace.BeginHost();
    m_MenuBar.Draw(m_Context, *this);
    m_Toolbar.Draw(m_Context, *this);
    m_Dockspace.SubmitDockSpace();
    m_Dockspace.EndHost();

    // Panels are independent windows that dock into the dockspace nodes.
    m_Context.panelManager.OnImGui(m_Context);

    DrawUnsavedPrompt();
}

int EditorApp::ValidateActiveScene() {
    if (m_Context.activeScene == nullptr) {
        HK_EDITOR_WARN("Validate Scene: no active scene");
        return 0;
    }
    const auto issues = SceneValidator::Validate(*m_Context.activeScene);
    for (const SceneValidationIssue& issue : issues) {
        if (issue.severity == SceneValidationIssue::Severity::Error) {
            HK_EDITOR_ERROR("[validation] {}", issue.message);
        } else {
            HK_EDITOR_WARN("[validation] {}", issue.message);
        }
    }
    HK_EDITOR_INFO("Validate Scene: {} issue(s)", issues.size());
    return static_cast<int>(issues.size());
}

void EditorApp::TogglePlayMode() {
    if (m_Context.activeScene == nullptr || m_Context.simulateMode) {
        return;
    }
    m_Context.playMode = !m_Context.playMode;
    if (m_Context.playMode) {
        m_Context.activeScene->SetMode(SceneMode::Play);
        m_Context.activeScene->OnRuntimeStart();
        HK_EDITOR_INFO("Enter Play mode");
    } else {
        m_Context.activeScene->OnRuntimeStop();
        m_Context.activeScene->SetMode(SceneMode::Edit);
        HK_EDITOR_INFO("Exit Play mode");
    }
}

void EditorApp::ToggleSimulateMode() {
    if (m_Context.activeScene == nullptr || m_Context.playMode) {
        return;
    }
    m_Context.simulateMode = !m_Context.simulateMode;
    if (m_Context.simulateMode) {
        m_Context.activeScene->OnSimulationStart();
        HK_EDITOR_INFO("Enter Simulate mode");
    } else {
        m_Context.activeScene->OnSimulationStop();
        HK_EDITOR_INFO("Exit Simulate mode");
    }
}

namespace {

// Returns selected ids whose ancestors are not also selected, so multi-entity
// operations act once per logical subtree (a parent already carries its kids).
std::vector<UUID> TopLevelSelected(Scene& scene, const Selection& selection) {
    std::vector<UUID> result;
    for (const UUID id : selection.All()) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity) {
            continue;
        }
        bool ancestorSelected = false;
        for (Entity parent = scene.GetParent(entity); parent; parent = scene.GetParent(parent)) {
            if (selection.IsSelected(parent.GetUUID())) {
                ancestorSelected = true;
                break;
            }
        }
        if (!ancestorSelected) {
            result.push_back(id);
        }
    }
    return result;
}

} // namespace

void EditorApp::Undo() {
    m_Context.undoRedo.Undo(m_Context);
}

void EditorApp::Redo() {
    m_Context.undoRedo.Redo(m_Context);
}

void EditorApp::CopySelection() {
    if (m_Context.activeScene == nullptr) {
        return;
    }
    m_Context.clipboard.CopyEntities(*m_Context.activeScene,
                                     TopLevelSelected(*m_Context.activeScene, m_Context.selection));
}

void EditorApp::CutSelection() {
    CopySelection();
    DeleteSelection();
}

void EditorApp::PasteFromClipboard(bool asChildOfSelection) {
    if (m_Context.activeScene == nullptr || !m_Context.clipboard.HasEntities()) {
        return;
    }
    const UUID parentId = asChildOfSelection ? m_Context.selection.Primary() : UUID(0);
    for (const std::string& snapshot : m_Context.clipboard.EntitySnapshots()) {
        m_Context.undoRedo.Execute(EditorCommands::PasteEntity(snapshot, parentId), m_Context);
    }
}

void EditorApp::DuplicateSelection() {
    if (m_Context.activeScene == nullptr) {
        return;
    }
    for (const UUID id : TopLevelSelected(*m_Context.activeScene, m_Context.selection)) {
        m_Context.undoRedo.Execute(EditorCommands::DuplicateEntity(id), m_Context);
    }
}

void EditorApp::DeleteSelection() {
    if (m_Context.activeScene == nullptr) {
        return;
    }
    for (const UUID id : TopLevelSelected(*m_Context.activeScene, m_Context.selection)) {
        m_Context.undoRedo.Execute(EditorCommands::DeleteEntity(*m_Context.activeScene, id), m_Context);
    }
}

std::filesystem::path EditorApp::DefaultSceneDirectory() const {
    if (!m_Context.activeScenePath.empty()) {
        return m_Context.activeScenePath.parent_path();
    }
    const std::filesystem::path scenes = Paths::Get().rawAssets / "scenes";
    return std::filesystem::exists(scenes) ? scenes : Paths::Get().rawAssets;
}

void EditorApp::RequestSceneAction(PendingAction action, const std::filesystem::path& path) {
    m_PendingAction = action;
    m_PendingScenePath = path;
    if (m_Context.sceneDirty) {
        m_OpenUnsavedPopup = true; // DrawUnsavedPrompt opens the modal next frame
    } else {
        PerformPendingAction();
    }
}

void EditorApp::PerformPendingAction() {
    const PendingAction action = m_PendingAction;
    const std::filesystem::path path = m_PendingScenePath;
    m_PendingAction = PendingAction::None;
    m_PendingScenePath.clear();

    // Any scene switch must end the preview first so its bodies/snapshot don't
    // outlive the scene they were built from.
    if (action != PendingAction::None && action != PendingAction::Quit && m_Context.activeScene != nullptr) {
        m_PhysicsPreview.Stop(*m_Context.activeScene);
        m_Context.physicsView.previewEnabled = false;
        m_Context.physicsView.previewRunning = false;
    }

    switch (action) {
    case PendingAction::None:
        break;
    case PendingAction::NewScene:
        m_SceneWorkflow.NewScene(m_Context);
        break;
    case PendingAction::OpenDialog:
        if (const auto chosen = FileDialog::OpenFile({{"Hockey Scene", "yaml"}}, DefaultSceneDirectory())) {
            DoOpenScene(*chosen);
        }
        break;
    case PendingAction::OpenPath:
        DoOpenScene(path);
        break;
    case PendingAction::Quit:
        m_WantsQuit = true;
        break;
    }
}

void EditorApp::DoOpenScene(const std::filesystem::path& path) {
    if (const Status status = m_SceneWorkflow.OpenScene(m_Context, path); !status) {
        HK_EDITOR_ERROR("Open scene '{}' failed: {}", path.string(), status.error);
    }
}

void EditorApp::NewScene() {
    RequestSceneAction(PendingAction::NewScene);
}

void EditorApp::OpenScene() {
    RequestSceneAction(PendingAction::OpenDialog);
}

void EditorApp::OpenScene(const std::filesystem::path& path) {
    RequestSceneAction(PendingAction::OpenPath, path);
}

bool EditorApp::SaveScene() {
    return DoSaveScene();
}

bool EditorApp::SaveSceneAs() {
    return DoSaveSceneAs();
}

bool EditorApp::DoSaveScene() {
    if (m_Context.activeScenePath.empty()) {
        return DoSaveSceneAs();
    }
    if (const Status status = m_SceneWorkflow.SaveScene(m_Context); !status) {
        HK_EDITOR_ERROR("Save scene failed: {}", status.error);
        return false;
    }
    HK_EDITOR_INFO("Saved scene to {}", m_Context.activeScenePath.string());
    return true;
}

bool EditorApp::DoSaveSceneAs() {
    std::string defaultName = "Untitled.scene.yaml";
    if (m_Context.activeScene != nullptr && !m_Context.activeScene->GetName().empty()) {
        defaultName = m_Context.activeScene->GetName() + ".scene.yaml";
    }
    const auto chosen = FileDialog::SaveFile({{"Hockey Scene", "yaml"}}, DefaultSceneDirectory(), defaultName);
    if (!chosen) {
        return false;
    }
    if (const Status status = m_SceneWorkflow.SaveSceneAs(m_Context, *chosen); !status) {
        HK_EDITOR_ERROR("Save scene failed: {}", status.error);
        return false;
    }
    HK_EDITOR_INFO("Saved scene to {}", m_Context.activeScenePath.string());
    return true;
}

void EditorApp::RequestQuit() {
    RequestSceneAction(PendingAction::Quit);
}

void EditorApp::OpenAutosaveFolder() {
    const std::filesystem::path dir = SceneWorkflow::AutosaveDirectory();
    FileSystem::CreateDirectories(dir);
    ProjectBrowser::Reveal(dir);
}

void EditorApp::DrawUnsavedPrompt() {
    constexpr const char* kPopupId = "Unsaved Changes##editor";
    if (m_OpenUnsavedPopup) {
        ImGui::OpenPopup(kPopupId);
        m_OpenUnsavedPopup = false;
    }

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::TextUnformatted("The current scene has unsaved changes.");
    ImGui::TextUnformatted("Save them before continuing?");
    ImGui::Separator();

    if (ImGui::Button("Save", ImVec2(110.0f, 0.0f))) {
        if (DoSaveScene()) {
            ImGui::CloseCurrentPopup();
            PerformPendingAction();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard", ImVec2(110.0f, 0.0f))) {
        ImGui::CloseCurrentPopup();
        PerformPendingAction();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f))) {
        ImGui::CloseCurrentPopup();
        m_PendingAction = PendingAction::None;
        m_PendingScenePath.clear();
    }
    ImGui::EndPopup();
}

void EditorApp::ProcessShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) {
        return; // never steal keys while editing a text field
    }
    const bool ctrl = io.KeyCtrl;
    const bool shift = io.KeyShift;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
        NewScene();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        OpenScene();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (shift) {
            SaveSceneAs();
        } else {
            SaveScene();
        }
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        if (shift) {
            Redo();
        } else {
            Undo();
        }
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        Redo();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        CopySelection();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        CutSelection();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        PasteFromClipboard(/*asChildOfSelection=*/false);
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        DuplicateSelection();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        DeleteSelection();
    }
}

} // namespace Hockey
