#include "MapEditorApp.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/Screenshot.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Renderer/RendererInitInfo.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"

#include <algorithm>
#include <filesystem>

namespace {

Hockey::RendererSettings LoadEditorViewportRendererSettings(const Hockey::Config& editorConfig) {
    Hockey::RendererSettings settings = Hockey::MakeDefaultRendererSettings();
    Hockey::LoadRendererSettings(editorConfig, settings);
    HK_EDITOR_INFO("Editor viewport graphics loaded from editor renderer config");
    return settings;
}

} // namespace

bool MapEditorApp::OnInit() {
    const auto& cmd = GetCommandLine();
    const std::filesystem::path root =
        cmd.Has("--root") ? std::filesystem::path(cmd.GetString("--root", "")) : std::filesystem::path{};
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

    m_FrameLimit = cmd.GetInt("--max-frames", cmd.GetInt("--frames", 0));
    m_AutoScreenshotPending = cmd.Has("--screenshot") || cmd.Has("--screenshot-window");
    m_ScreenshotFrame = std::max(1, cmd.GetInt("--screenshot-frame", 3));
    m_ScreenshotPrefix = cmd.GetString("--screenshot-prefix", cmd.GetString("--screenshot", "editor_window"));
    if (m_ScreenshotPrefix.empty() || m_ScreenshotPrefix == "true") {
        m_ScreenshotPrefix = "editor_window";
    }

    // Bring up the physics engine so editor playtest can simulate and Scene
    // View can draw collider/trigger debug geometry. The editor never
    // auto-simulates an Edit-mode scene outside playtest.
    if (m_Config.GetBool("physics.enabled", true)) {
        if (!Hockey::Physics::Init()) {
            HK_EDITOR_INFO("Physics failed to initialise; editor running without physics");
        } else {
            m_PhysicsReady = true;
            HK_EDITOR_INFO("Physics initialised for editor playtest/debug visualization");
        }
    }

    m_Scene.SetMode(Hockey::SceneMode::Edit);

    Hockey::RendererInitInfo rendererInfo;
    rendererInfo.window = &GetWindow();
    rendererInfo.settings = LoadEditorViewportRendererSettings(m_Config);
    rendererInfo.enableValidation = GetCommandLine().Has("--vk-validation");
    rendererInfo.shaderSourceDirectory = Hockey::Paths::Get().root / "data/shaders/src";
    rendererInfo.shaderBinaryDirectory = Hockey::Paths::Get().root / "data/shaders/bin";
    if (const Hockey::Status status = m_Renderer.Init(rendererInfo); !status) {
        HK_CORE_ERROR("Renderer init failed: {}", status.error);
        return false;
    }

    // Content pipeline: load the existing cooked asset database up front so the
    // editor can resolve AssetIDs quickly. Full discovery/import/cook work is
    // opt-in because it can dominate editor startup on large asset folders.
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
    const bool autoDiscover = m_Config.GetBool("assets.auto_discover", false);
    const bool autoImport = m_Config.GetBool("assets.auto_import", false);
    const bool autoCookDirty = m_Config.GetBool("assets.auto_cook_dirty", false);
    const bool hotReload = m_Config.GetBool("assets.hot_reload", false);

    if (autoDiscover) {
        if (const Hockey::Status status = m_AssetManager.DiscoverRawAssets(); !status) {
            HK_EDITOR_WARN("Startup asset discovery failed: {}", status.error);
        }
    }
    if (autoImport) {
        if (const Hockey::Status status = m_AssetManager.ImportAll(); !status) {
            HK_EDITOR_WARN("Startup asset import reported issues: {}", status.error);
        }
    }
    if (autoCookDirty) {
        if (const Hockey::Status status = m_AssetManager.CookAllDirty(); !status) {
            HK_EDITOR_WARN("Startup asset cook reported issues: {}", status.error);
        }
    }
    if (hotReload) {
        m_AssetManager.StartWatching();
    } else {
        HK_EDITOR_INFO("Startup asset scan/import/cook/hot-reload disabled; use the Project panel to refresh assets");
    }
    m_Renderer.SetAssetManager(&m_AssetManager);

    Hockey::EditorContextCreateInfo editorInfo;
    editorInfo.window = &GetWindow();
    editorInfo.renderer = &m_Renderer;
    editorInfo.scene = &m_Scene;
    editorInfo.config = &m_Config;
    editorInfo.configPath = configPath;
    editorInfo.assetManager = &m_AssetManager;
    editorInfo.captureSceneViewport = cmd.Has("--capture-viewports") || cmd.Has("--capture-scene-view");
    editorInfo.captureGameViewport = cmd.Has("--capture-viewports") || cmd.Has("--capture-game-view");
    editorInfo.captureViewportPrefix =
        cmd.GetString("--capture-prefix", cmd.GetString("--screenshot-prefix", "editor"));
    editorInfo.captureViewportWidth = static_cast<std::uint32_t>(std::max(1, cmd.GetInt("--capture-width", 1920)));
    editorInfo.captureViewportHeight = static_cast<std::uint32_t>(std::max(1, cmd.GetInt("--capture-height", 1080)));
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
    std::filesystem::path path;
    if (cmd.Has("--scene")) {
        path = Hockey::Paths::Resolve(cmd.GetString("--scene", ""));
    } else {
        const Hockey::EditorSettings& settings = m_Editor.Context().settings;
        if (settings.restoreLastScene && !settings.recentScenes.empty()) {
            const std::filesystem::path recent = Hockey::Paths::Resolve(settings.recentScenes.front());
            if (Hockey::FileSystem::Exists(recent)) {
                path = recent;
                HK_EDITOR_INFO("Restoring last scene '{}'", path.string());
            } else {
                HK_EDITOR_INFO("Last scene '{}' not found; opening empty Edit scene", recent.string());
            }
        }
    }
    if (path.empty()) {
        HK_EDITOR_INFO("No startup scene requested; opening empty Edit scene");
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
    const int renderedFrame = m_FrameCount + 1;
    if (m_AutoScreenshotPending && renderedFrame >= m_ScreenshotFrame) {
        const std::filesystem::path path = Hockey::MakeScreenshotPath(m_ScreenshotPrefix);
        if (const Hockey::Status status = m_Renderer.RequestScreenshot(path); !status) {
            HK_EDITOR_INFO("Screenshot request failed: {}", status.error);
        } else {
            HK_EDITOR_INFO("Screenshot queued: {}", path.string());
        }
        m_AutoScreenshotPending = false;
    }
    m_Editor.EndFrame();

    if (m_Editor.WantsQuit()) {
        RequestQuit();
    }
    ++m_FrameCount;
    if (m_FrameLimit > 0 && m_FrameCount >= m_FrameLimit) {
        RequestQuit();
    }
}
