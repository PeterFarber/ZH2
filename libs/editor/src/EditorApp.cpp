#include "Hockey/Editor/EditorApp.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetEvent.hpp"
#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
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
#include "Hockey/Editor/EditorClientPreview.hpp"
#include "Hockey/Editor/FileDialog.hpp"
#include "Hockey/Editor/Logging/EditorConsoleSink.hpp"
#include "Hockey/Editor/Panels/ClientFlowPanel.hpp"
#include "Hockey/Editor/Panels/ClientPreviewPanel.hpp"
#include "Hockey/Editor/Panels/ConsolePanel.hpp"
#include "Hockey/Editor/Panels/GameViewportPanel.hpp"
#include "Hockey/Editor/Panels/GameplayTuningPanel.hpp"
#include "Hockey/Editor/Panels/HierarchyPanel.hpp"
#include "Hockey/Editor/Panels/InspectorPanel.hpp"
#include "Hockey/Editor/Panels/PackagePanel.hpp"
#include "Hockey/Editor/Panels/PrefabPanel.hpp"
#include "Hockey/Editor/Panels/ProjectPanel.hpp"
#include "Hockey/Editor/Panels/ProjectSettingsPanel.hpp"
#include "Hockey/Editor/Panels/PropertiesPanel.hpp"
#include "Hockey/Editor/Panels/SceneValidationPanel.hpp"
#include "Hockey/Editor/Panels/SceneViewportPanel.hpp"
#include "Hockey/Editor/Panels/StatsPanel.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsMeshProvider.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

// Returns a path inside 'dir' for 'fileName' that does not collide with an
// existing file, inserting "_N" before the first '.' so compound extensions
// (e.g. ".material.yaml") stay intact: "puck.material.yaml" -> "puck_1.material.yaml".
std::filesystem::path UniqueDestination(const std::filesystem::path& dir, const std::string& fileName) {
    std::filesystem::path candidate = dir / fileName;
    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec)) {
        return candidate;
    }
    const std::size_t dot = fileName.find('.');
    const std::string stem = dot == std::string::npos ? fileName : fileName.substr(0, dot);
    const std::string exts = dot == std::string::npos ? std::string{} : fileName.substr(dot);
    for (int n = 1; n < 10000; ++n) {
        candidate = dir / (stem + "_" + std::to_string(n) + exts);
        if (!std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return candidate;
}

} // namespace

const char* EditorVersionString() {
    return "HockeyEditor 0.4.0";
}

EditorApp::EditorApp() : m_ClientPreview(std::make_unique<EditorClientPreview>()) {}

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
    m_Context.configPath = info.configPath;
    m_Context.assetManager = info.assetManager;
    m_Context.captureSceneViewport = info.captureSceneViewport;
    m_Context.captureGameViewport = info.captureGameViewport;
    m_Context.captureViewportPrefix =
        info.captureViewportPrefix.empty() ? std::string("editor") : info.captureViewportPrefix;
    m_Context.captureViewportWidth = std::max(1u, info.captureViewportWidth);
    m_Context.captureViewportHeight = std::max(1u, info.captureViewportHeight);
    // Seed autosave/asset defaults from editor.toml, then let the user's
    // editor_settings.toml override where present.
    if (m_Context.config != nullptr) {
        m_Context.settings.ApplyProjectConfig(*m_Context.config);
    }
    m_Context.settings.Load(EditorSettings::DefaultPath());

    // Mirror engine log output into the Console panel.
    InstallEditorConsoleSink();

    if (Status s = m_ImGuiLayer.Init(*info.window, *info.renderer, m_Context.settings.editorScale); !s) {
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
    RegisterGameplayComponents();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    // hockey_physics cannot load mesh assets itself (it must not depend on
    // hockey_assets). Install a provider that resolves MeshColliderComponent
    // geometry from the editor's AssetManager so the physics preview can build
    // mesh-collider shapes. Positions/indices are taken from the cooked mesh.
    if (m_Context.assetManager != nullptr) {
        AssetManager* assets = m_Context.assetManager;
        PhysicsMeshRegistry::Get().SetProvider([assets](std::uint64_t meshAsset, PhysicsMeshData& out) -> bool {
            Result<std::shared_ptr<MeshAsset>> loaded = assets->Load<MeshAsset>(AssetID{meshAsset});
            if (!loaded || !loaded.value) {
                return false;
            }
            const MeshAsset& mesh = *loaded.value;
            out.vertices.clear();
            out.vertices.reserve(mesh.vertices.size());
            for (const MeshVertex& v : mesh.vertices) {
                out.vertices.push_back(v.position);
            }
            out.indices = mesh.indices;
            return !out.vertices.empty();
        });
    }

    // Wire the non-destructive physics preview (Edit mode never auto-simulates).
    PhysicsSettings physicsSettings;
    if (m_Context.config != nullptr) {
        LoadPhysicsSettings(*m_Context.config, physicsSettings);
    }
    m_PhysicsPreview.Configure(physicsSettings);
    m_Context.physicsPreview = &m_PhysicsPreview;

    GameplaySettings gameplaySettings;
    if (m_Context.config != nullptr) {
        gameplaySettings = LoadGameplaySettings(*m_Context.config);
    }
    GameplayTuning gameplayTuning;
    if (const Result<GameplayTuning> loaded = TuningSerializer::Load(Paths::DataFile("gameplay/tuning.default.yaml"))) {
        gameplayTuning = loaded.value;
    } else {
        HK_EDITOR_WARN("Gameplay tuning load failed: {}. Using built-in defaults.", loaded.error);
    }
    m_GameplayPreview.Configure(gameplaySettings, gameplayTuning);
    m_Context.gameplayPreview = &m_GameplayPreview;
    m_Context.clientPreviewHost = m_ClientPreview.get();

    RegisterPanels();
    m_Context.panelManager.ApplyPanelOpenStates(m_Context.settings);
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
    panels.AddPanel<ClientPreviewPanel>();
    panels.AddPanel<GameplayTuningPanel>();
    panels.AddPanel<ProjectPanel>();
    panels.AddPanel<ClientFlowPanel>();
    panels.AddPanel<ConsolePanel>();
    panels.AddPanel<PropertiesPanel>();
    panels.AddPanel<StatsPanel>();
    panels.AddPanel<SceneValidationPanel>();
    panels.AddPanel<PrefabPanel>();
    panels.AddPanel<ProjectSettingsPanel>();
    panels.AddPanel<PackagePanel>();
}

void EditorApp::RegisterTools() {
    RegisterEditorTools(m_Context.toolManager);
    // Default to the Move tool so the gizmo starts in translate mode.
    m_Context.toolManager.Activate("Move", m_Context);
}

void EditorApp::CapturePanelLayoutState() {
    m_Context.settings.SetPanelOpenStates(m_Context.panelManager.CapturePanelOpenStates());
}

void EditorApp::SaveLayout() {
    if (!m_Initialized) {
        return;
    }
    CapturePanelLayoutState();
    if (const Status status = m_Context.settings.Save(EditorSettings::DefaultPath()); !status) {
        HK_EDITOR_WARN("Saving editor settings failed: {}", status.error);
    }
    m_ImGuiLayer.SaveLayout();
    HK_EDITOR_INFO("Saved editor layout ({})", m_ImGuiLayer.IniPath());
}

void EditorApp::SetEditorScale(float scale) {
    const float normalizedScale = EditorSettings::NormalizeEditorScale(scale);
    if (m_Context.settings.editorScale == normalizedScale) {
        return;
    }

    m_Context.settings.editorScale = normalizedScale;
    m_ImGuiLayer.ApplyEditorScale(normalizedScale);
    if (const Status status = m_Context.settings.Save(EditorSettings::DefaultPath()); !status) {
        HK_EDITOR_WARN("Saving editor scale failed: {}", status.error);
    }
}

void EditorApp::ResetLayout() {
    m_Context.panelManager.ResetPanelOpenStates();
    CapturePanelLayoutState();
    if (const Status status = m_Context.settings.Save(EditorSettings::DefaultPath()); !status) {
        HK_EDITOR_WARN("Saving editor settings after layout reset failed: {}", status.error);
    }
    m_Dockspace.RequestReset();
    HK_EDITOR_INFO("Reset editor layout to defaults");
}

void EditorApp::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    StopClientPreviewMode();
    // Tear down any active physics preview, restoring the authored transforms.
    if (m_Context.activeScene != nullptr) {
        m_GameplayPreview.Stop(*m_Context.activeScene, m_PhysicsPreview);
        m_PhysicsPreview.Stop(*m_Context.activeScene);
    }
    m_Context.clientPreviewHost = nullptr;
    m_Context.gameplayPreview = nullptr;
    m_Context.physicsPreview = nullptr;
    // Drop the mesh-collider provider so it can't outlive the AssetManager.
    PhysicsMeshRegistry::Get().Clear();
    SaveLayout();
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
    if (m_Context.requestClientPreviewStart) {
        m_Context.requestClientPreviewStart = false;
        StartClientPreviewMode();
    }

    PollAssetHotReload();

    m_Context.toolManager.Update(m_Context, deltaTime);
    m_SceneWorkflow.TickAutosave(m_Context, deltaTime);

    // Advance the physics preview (no-op unless the user enabled + is playing).
    if (m_Context.activeScene != nullptr) {
        const bool playtestInputActive = m_Context.playMode && m_Context.gameplayView.gameInputActive;
        const bool clientPreviewInputActive = m_ClientPreview != nullptr && m_ClientPreview->IsRunning() &&
                                              m_Context.clientPreview.gameInputActive;
        m_GameplayPreview.SetInputEnabled(playtestInputActive || clientPreviewInputActive);
        if (m_GameplayPreview.IsActive()) {
            m_GameplayPreview.Update(*m_Context.activeScene, m_PhysicsPreview, deltaTime);
        } else {
            m_PhysicsPreview.Update(*m_Context.activeScene, deltaTime);
        }
        SyncPreviewState();
        m_Context.physicsView.contactPoints.clear();
        if (m_Context.physicsView.showContacts && m_PhysicsPreview.IsActive()) {
            m_Context.physicsView.contactPoints = m_PhysicsPreview.ContactPoints();
        }
    }
    if (m_ClientPreview != nullptr && m_ClientPreview->IsActive()) {
        m_ClientPreview->Update(m_Context, deltaTime);
        SyncPreviewState();
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

void EditorApp::TogglePlaytestMode() {
    if (m_Context.playMode) {
        StopPlaytestMode();
        return;
    }
    StopClientPreviewMode();

    if (m_Context.activeScene == nullptr) {
        return;
    }

    m_PlayModeRestoreDirty = m_Context.sceneDirty;
    if (const Status status = m_GameplayPreview.Start(*m_Context.activeScene, m_PhysicsPreview); !status) {
        HK_EDITOR_ERROR("Playtest failed: {}", status.error);
        SyncPreviewState();
        return;
    }

    m_Context.playMode = true;
    m_Context.gameplayView.previewEnabled = true;
    m_Context.gameplayView.gameInputActive = false;
    m_GameplayPreview.SetDebugDrawEnabled(m_Context.gameplayView.showDebug);
    m_GameplayPreview.SetInputEnabled(false);
    m_GameplayPreview.Play();
    SyncPreviewState();
    m_Context.panelManager.RequestPanelFocus(m_Context, EditorPanelNames::kGameViewport);
    HK_EDITOR_INFO("Enter Play mode");
}

void EditorApp::ToggleClientPreviewMode() {
    if (m_ClientPreview != nullptr && m_ClientPreview->IsActive()) {
        StopClientPreviewMode();
        return;
    }
    StartClientPreviewMode();
}

void EditorApp::StartClientPreviewMode() {
    if (m_ClientPreview == nullptr) {
        return;
    }
    if (m_ClientPreview->IsActive()) {
        StopClientPreviewMode();
    }
    StopPlaytestMode();
    if (m_Context.activeScene == nullptr) {
        return;
    }
    if (const Status status = m_ClientPreview->Start(m_Context, m_Context.clientFlowAssetPath); !status) {
        HK_EDITOR_ERROR("Client Preview failed: {}", status.error);
        SyncPreviewState();
        return;
    }
    SyncPreviewState();
    m_Context.panelManager.RequestPanelFocus(m_Context, EditorPanelNames::kClientPreview);
    HK_EDITOR_INFO("Start Client Preview");
}

void EditorApp::StopClientPreviewMode() {
    if (m_ClientPreview == nullptr || !m_ClientPreview->IsActive()) {
        SyncPreviewState();
        return;
    }
    m_ClientPreview->Stop(m_Context);
    SyncPreviewState();
    HK_EDITOR_INFO("Stop Client Preview");
}

void EditorApp::StopPlaytestMode() {
    const bool wasPlaying = m_Context.playMode;
    const bool hadPreview = m_GameplayPreview.IsActive() || m_PhysicsPreview.IsActive();
    if (!wasPlaying && !hadPreview) {
        return;
    }

    if (m_Context.activeScene != nullptr) {
        m_GameplayPreview.Stop(*m_Context.activeScene, m_PhysicsPreview);
        m_PhysicsPreview.Stop(*m_Context.activeScene);
    }
    m_GameplayPreview.SetInputEnabled(false);
    m_Context.playMode = false;
    m_Context.gameplayView.gameInputActive = false;
    m_Context.physicsView.contactPoints.clear();
    if (wasPlaying) {
        m_Context.sceneDirty = m_PlayModeRestoreDirty;
    }
    SyncPreviewState();
    if (wasPlaying) {
        HK_EDITOR_INFO("Exit Play mode");
    }
}

void EditorApp::SyncPreviewState() {
    m_Context.gameplayView.previewEnabled = m_GameplayPreview.IsActive();
    m_Context.gameplayView.previewRunning = m_GameplayPreview.IsRunning();
    m_Context.physicsView.previewEnabled = m_PhysicsPreview.IsActive();
    m_Context.physicsView.previewRunning = m_PhysicsPreview.IsRunning();
    m_Context.clientPreview.previewEnabled = m_ClientPreview != nullptr && m_ClientPreview->IsActive();
    m_Context.clientPreview.previewRunning = m_ClientPreview != nullptr && m_ClientPreview->IsRunning();
    if (!m_Context.playMode) {
        m_Context.gameplayView.gameInputActive = false;
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
    if (m_Context.playMode) {
        return;
    }
    m_Context.undoRedo.Undo(m_Context);
}

void EditorApp::Redo() {
    if (m_Context.playMode) {
        return;
    }
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

void EditorApp::SelectAllEntities() {
    if (m_Context.activeScene == nullptr) {
        return;
    }
    m_Context.SelectAllSceneEntities(*m_Context.activeScene);
}

void EditorApp::DeselectAll() {
    m_Context.selection.Clear();
}

void EditorApp::OpenProjectSettings() {
    m_Context.panelManager.RequestPanelFocus(m_Context, EditorPanelNames::kProjectSettings);
}

void EditorApp::OpenPackageWindow() {
    m_Context.panelManager.RequestPanelFocus(m_Context, EditorPanelNames::kPackage);
}

void EditorApp::OpenPreferences() {
    OpenProjectSettings();
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
        StopClientPreviewMode();
        StopPlaytestMode();
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
    if (m_Context.playMode) {
        HK_EDITOR_WARN("Exit Play mode before saving the scene");
        return false;
    }
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
    if (m_Context.playMode) {
        HK_EDITOR_WARN("Exit Play mode before saving the scene");
        return false;
    }
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

void EditorApp::ImportAsset() {
    if (m_Context.assetManager == nullptr) {
        HK_EDITOR_ERROR("Import asset: no asset manager is available");
        return;
    }
    // One combined filter (so any importable file is selectable) plus per-type
    // filters for convenience. Extensions are dot-less, comma-separated.
    const auto chosen = FileDialog::OpenFile(
        {
            {"All importable", "gltf,glb,png,jpg,jpeg,tga,bmp,hdr,ktx,ktx2,yaml,glsl,vert,frag,comp"},
            {"Models", "gltf,glb"},
            {"Textures", "png,jpg,jpeg,tga,bmp,hdr,ktx,ktx2"},
            {"Materials / Scenes / Prefabs", "yaml"},
            {"Shaders", "glsl,vert,frag,comp"},
        },
        {});
    if (!chosen) {
        return; // user cancelled (or no native dialog available)
    }
    DoImportAsset(*chosen);
}

void EditorApp::DoImportAsset(const std::filesystem::path& source) {
    AssetManager* assets = m_Context.assetManager;
    if (assets == nullptr) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(source, ec)) {
        HK_EDITOR_ERROR("Import asset: file not found '{}'", source.string());
        return;
    }

    const AssetType type = AssetManager::ClassifyExtension(source);
    if (type == AssetType::Unknown) {
        HK_EDITOR_ERROR("Import asset: unsupported file type '{}'", source.filename().string());
        return;
    }

    // Place the file under data/raw/<type> using the same folder names the cooker
    // uses (materials/models/textures/...). DiscoverRawAssets scans recursively,
    // so the exact subfolder is convention, but keeping it tidy helps the user.
    const std::filesystem::path destDir = assets->Info().rawRoot / AssetPath::CookedSubdirectory(type);
    if (const Status created = FileSystem::CreateDirectories(destDir); !created) {
        HK_EDITOR_ERROR("Import asset: cannot create '{}': {}", destDir.string(), created.error);
        return;
    }

    const std::filesystem::path dest = UniqueDestination(destDir, source.filename().string());
    std::filesystem::copy_file(source, dest, std::filesystem::copy_options::none, ec);
    if (ec) {
        HK_EDITOR_ERROR("Import asset: failed to copy '{}' -> '{}': {}", source.string(), dest.string(),
                        ec.message());
        return;
    }

    // Import the new file, then discover so any sibling files an importer wrote
    // (e.g. textures unpacked from a GLB) are registered, then cook just the dirty
    // assets (the new file, its sub-assets and those textures) in dependency order.
    if (const Status status = assets->ImportAsset(dest); !status) {
        HK_EDITOR_ERROR("Import asset: import of '{}' failed: {}", dest.filename().string(), status.error);
        std::filesystem::remove(dest, ec);
        return;
    }
    assets->DiscoverRawAssets();
    if (const Status status = assets->CookAllDirty(); !status) {
        HK_EDITOR_WARN("Import asset: cook reported issues: {}", status.error);
    }
    assets->SaveDatabase();

    // Drop any renderer caches for assets that were (re)cooked so they appear
    // immediately, regardless of whether hot reload is enabled.
    if (m_Context.renderer != nullptr) {
        const std::vector<AssetEvent>& events = assets->PollEvents();
        for (const AssetEvent& event : events) {
            m_Context.renderer->InvalidateAsset(event.id.Value());
        }
    }

    HK_EDITOR_INFO("Imported '{}' into '{}'", source.filename().string(),
                   AssetPath::CookedSubdirectory(type));
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

    if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
        if (m_Context.activeScene != nullptr) {
            m_Context.undoRedo.Execute(
                EditorCommands::CreateEntity("GameObject", m_Context.DefaultParentFor(*m_Context.activeScene)),
                m_Context);
        }
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
        NewScene();
    }
    if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_G, false)) {
        if (m_Context.activeScene != nullptr &&
            EditorCommands::CanCreateEmptyParent(*m_Context.activeScene, m_Context.selection.All())) {
            m_Context.undoRedo.Execute(
                EditorCommands::CreateEmptyParent(*m_Context.activeScene, m_Context.selection.All()), m_Context);
        }
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
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
        SelectAllEntities();
    }
    if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
        ToggleClientPreviewMode();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
        TogglePlaytestMode();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        DeleteSelection();
    }
    if (!m_Context.selection.Empty() && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        DeselectAll();
    }
}

} // namespace Hockey
