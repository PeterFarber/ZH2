#include "MapEditorApp.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Renderer/RendererInitInfo.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"

bool MapEditorApp::OnInit() {
    const auto& cmd = GetCommandLine();
    const auto root = cmd.GetString("--root", ".");
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    const auto logPath = cmd.Has("--log") ? Hockey::Paths::Resolve(cmd.GetString("--log"))
                                          : Hockey::Paths::LogFile("editor.log");
    Hockey::Log::Init(logPath);
    Hockey::CrashHandler::Install();
    Hockey::JobSystem::Init();
    const auto configPath = cmd.Has("--config") ? Hockey::Paths::Resolve(cmd.GetString("--config"))
                                                : Hockey::Paths::ConfigFile("editor.toml");
    m_Config.Load(configPath);
    Hockey::Input::SetGamepadEnabled(m_Config.GetBool("input.gamepad_enabled", true));

    if (!CreateAppWindowFromConfig(m_Config)) {
        HK_CORE_ERROR("Failed to create map editor window");
        return false;
    }
    HK_EDITOR_INFO("Map editor started on {} ({} hardware threads)", Hockey::Platform::OSName(),
                   Hockey::Platform::HardwareThreadCount());

    m_FrameLimit = GetCommandLine().GetInt("--frames", 0);

    // Bring up the physics engine so the editor can build preview bodies and
    // draw collider/trigger debug geometry. Preview simulation stays off until
    // the user enables it; the editor never auto-simulates an Edit-mode scene.
    if (m_Config.GetBool("physics.enabled", true)) {
        if (!Hockey::Physics::Init()) {
            HK_EDITOR_INFO("Physics failed to initialise; editor running without physics");
        } else {
            m_PhysicsReady = true;
            HK_EDITOR_INFO("Physics initialised for editor preview");
        }
    }

    m_Scene.SetMode(Hockey::SceneMode::Edit);

    Hockey::RendererInitInfo rendererInfo;
    rendererInfo.window = &GetWindow();
    rendererInfo.settings = Hockey::MakeDefaultRendererSettings();
    Hockey::LoadRendererSettings(m_Config, rendererInfo.settings);
    rendererInfo.enableValidation = GetCommandLine().Has("--vk-validation");
    rendererInfo.shaderSourceDirectory = Hockey::Paths::Get().root / "data/shaders/src";
    rendererInfo.shaderBinaryDirectory = Hockey::Paths::Get().root / "data/shaders/bin";
    if (const Hockey::Status status = m_Renderer.Init(rendererInfo); !status) {
        HK_CORE_ERROR("Renderer init failed: {}", status.error);
        return false;
    }

    // Content pipeline: discover/import/cook raw assets up front so the editor
    // and renderer can resolve AssetIDs, then watch for hot-reload changes. The
    // renderer borrows the manager to upload cooked meshes/materials/textures.
    // The raw/cooked roots default to the standard data/ layout but can be
    // redirected via the [assets] config keys.
    Hockey::AssetManagerCreateInfo assetInfo =
        Hockey::AssetManager::DefaultCreateInfo(Hockey::Paths::Get().root);
    if (const std::string rawOverride = m_Config.GetString("assets.raw_path", ""); !rawOverride.empty()) {
        assetInfo.rawRoot = Hockey::Paths::Resolve(rawOverride);
    }
    if (const std::string cookedOverride = m_Config.GetString("assets.cooked_path", ""); !cookedOverride.empty()) {
        assetInfo.cookedRoot = Hockey::Paths::Resolve(cookedOverride);
    }
    if (const Hockey::Status status = m_AssetManager.Init(assetInfo); !status) {
        HK_CORE_ERROR("Asset manager init failed: {}", status.error);
        return false;
    }
    // Honor the [assets] auto-pipeline config keys (mirrored by EditorSettings).
    if (m_Config.GetBool("assets.auto_discover", true)) {
        m_AssetManager.DiscoverRawAssets();
    }
    if (m_Config.GetBool("assets.auto_import", true)) {
        m_AssetManager.ImportAll();
    }
    if (m_Config.GetBool("assets.auto_cook_dirty", true)) {
        m_AssetManager.CookAllDirty();
    }
    m_AssetManager.SaveDatabase();
    m_AssetManager.StartWatching();
    m_Renderer.SetAssetManager(&m_AssetManager);

    Hockey::EditorContextCreateInfo editorInfo;
    editorInfo.window = &GetWindow();
    editorInfo.renderer = &m_Renderer;
    editorInfo.scene = &m_Scene;
    editorInfo.config = &m_Config;
    editorInfo.assetManager = &m_AssetManager;
    if (const Hockey::Status status = m_Editor.Init(editorInfo); !status) {
        HK_CORE_ERROR("Editor init failed: {}", status.error);
        return false;
    }

    // Feed raw SDL events to the editor's ImGui backend. The engine event queue
    // continues to drive core Input in parallel.
    GetWindow().SetRawEventObserver([this](const void* sdlEvent) { m_Editor.ProcessRawSDLEvent(sdlEvent); });

    LoadStartupScene();

    m_Ready = true;
    return true;
}

void MapEditorApp::LoadStartupScene() {
    const auto& cmd = GetCommandLine();
    const Hockey::EditorSettings& settings = m_Editor.Context().settings;

    // Precedence: explicit --scene > restore-last-scene (most recent) > config.
    std::filesystem::path path;
    if (cmd.Has("--scene")) {
        path = Hockey::Paths::Resolve(cmd.GetString("--scene", ""));
    } else if (settings.restoreLastScene && !settings.recentScenes.empty()) {
        const std::filesystem::path recent = Hockey::Paths::Resolve(settings.recentScenes.front());
        if (Hockey::FileSystem::Exists(recent)) {
            path = recent;
            HK_EDITOR_INFO("Restoring last scene '{}'", path.string());
        }
    }
    if (path.empty()) {
        const std::string startupScene = m_Config.GetString("scene.startup_scene", "");
        if (!startupScene.empty()) {
            path = Hockey::Paths::Resolve(startupScene);
        }
    }
    if (path.empty()) {
        return;
    }
    if (!Hockey::FileSystem::Exists(path)) {
        HK_EDITOR_INFO("Startup scene '{}' not found; using empty Edit scene", path.string());
        return;
    }

    // Scene loading (deserialize, active-path/recents/undo/dirty bookkeeping)
    // lives in the editor; the host just resolves and requests the path.
    m_Editor.OpenScene(path);
}

void MapEditorApp::OnShutdown() {
    HK_EDITOR_INFO("Shutdown");
    if (m_Ready) {
        GetWindow().SetRawEventObserver({});
        // Drain the GPU before ImGui destroys its Vulkan objects, then tear down
        // the editor before the renderer it depends on.
        m_Renderer.WaitIdle();
        m_Editor.Shutdown();
        // The renderer borrows the asset manager; stop using it before teardown.
        m_Renderer.SetAssetManager(nullptr);
        m_AssetManager.StopWatching();
        m_AssetManager.SaveDatabase();
        m_AssetManager.Shutdown();
        m_Renderer.Shutdown();
        m_Ready = false;
    }
    if (m_PhysicsReady) {
        // All editor-owned physics worlds are gone once the editor shut down, so
        // it is safe to tear down the global Jolt state.
        Hockey::Physics::Shutdown();
        m_PhysicsReady = false;
    }
    Hockey::JobSystem::Shutdown();
    Hockey::CrashHandler::Shutdown();
    Hockey::Log::Shutdown();
}

void MapEditorApp::OnUpdate(float deltaTime) {
    if (!m_Ready) {
        return;
    }
    m_Editor.BeginFrame(deltaTime);
    m_Editor.Update(deltaTime);
    m_Editor.Render();
    m_Editor.EndFrame();

    if (m_Editor.WantsQuit()) {
        RequestQuit();
    }
    if (m_FrameLimit > 0 && ++m_FrameCount >= m_FrameLimit) {
        RequestQuit();
    }
}
