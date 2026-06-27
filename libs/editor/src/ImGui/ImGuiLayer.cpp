#include "Hockey/Editor/ImGui/ImGuiLayer.hpp"

#include <filesystem>
#include <system_error>

#include <imgui.h>

#include <IconsFontAwesome.h>
#include <imgui_impl_sdl3.h>

#include <ImGuizmo.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/ImGui/EditorTheme.hpp"

namespace Hockey {
namespace {

constexpr float kEditorFontSizePixels = 13.0f;

} // namespace

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

Status ImGuiLayer::Init(Window& window, Renderer& renderer, float editorScale) {
    if (m_Initialized) {
        return Status::Ok();
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    LoadEditorFonts();

    // Persist the docking layout alongside the editor settings (data/editor).
    const std::filesystem::path layoutPath = Paths::DataFile("editor/layout.ini");
    std::error_code ec;
    std::filesystem::create_directories(layoutPath.parent_path(), ec);
    m_IniPath = layoutPath.string();
    io.IniFilename = m_IniPath.c_str();

    ApplyEditorScale(editorScale);

    if (!ImGui_ImplSDL3_InitForVulkan(window.SDLHandle())) {
        ImGui::DestroyContext();
        return Status::Fail("ImGui_ImplSDL3_InitForVulkan failed");
    }
    m_PlatformInitialized = true;

    if (Status s = m_Bridge.Init(renderer); !s) {
        ImGui_ImplSDL3_Shutdown();
        m_PlatformInitialized = false;
        ImGui::DestroyContext();
        return s;
    }

    m_Initialized = true;
    HK_EDITOR_INFO("ImGui layer initialized (layout: {})", m_IniPath);
    return Status::Ok();
}

void ImGuiLayer::SaveLayout() const {
    if (ImGui::GetCurrentContext() == nullptr || m_IniPath.empty()) {
        return;
    }
    ImGui::SaveIniSettingsToDisk(m_IniPath.c_str());
}

void ImGuiLayer::ApplyEditorScale(float editorScale) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    const float normalizedScale = EditorSettings::NormalizeEditorScale(editorScale);
    ImGui::GetIO().FontGlobalScale = normalizedScale;
    EditorTheme::ApplyDark(normalizedScale);
}

void ImGuiLayer::Shutdown() {
    if (!m_Initialized && !m_PlatformInitialized) {
        return;
    }
    m_Bridge.Shutdown();
    if (m_PlatformInitialized) {
        ImGui_ImplSDL3_Shutdown();
        m_PlatformInitialized = false;
    }
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }
    m_Initialized = false;
    m_IconFontLoaded = false;
}

void ImGuiLayer::LoadEditorFonts() {
    m_IconFontLoaded = false;

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig defaultFontConfig{};
    defaultFontConfig.SizePixels = kEditorFontSizePixels;
    defaultFontConfig.PixelSnapH = true;
    io.Fonts->AddFontDefault(&defaultFontConfig);

    const std::filesystem::path fontPath = Paths::DataFile("editor/fonts/fontawesome-free/fa-solid-900.ttf");
    if (!std::filesystem::exists(fontPath)) {
        HK_EDITOR_WARN("Font Awesome editor icon font was not found at {}; using text labels", fontPath.string());
        return;
    }

    ImFontConfig fontConfig{};
    fontConfig.MergeMode = true;
    fontConfig.PixelSnapH = true;
    fontConfig.GlyphMinAdvanceX = kEditorFontSizePixels;

    static constexpr ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFont* iconFont =
        io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), kEditorFontSizePixels, &fontConfig, iconRanges);
    if (iconFont == nullptr) {
        HK_EDITOR_WARN("Font Awesome editor icon font failed to load from {}; using text labels", fontPath.string());
        return;
    }

    m_IconFontLoaded = true;
}

bool ImGuiLayer::ProcessEvent(const void* sdlEvent) {
    if (!m_PlatformInitialized || sdlEvent == nullptr) {
        return false;
    }
    return ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(sdlEvent));
}

void ImGuiLayer::BeginFrame() {
    if (!m_Initialized) {
        return;
    }
    m_Bridge.NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiLayer::EndFrame() {
    if (!m_Initialized) {
        return;
    }
    ImGui::Render();
    m_Bridge.RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::WantsCaptureMouse() const {
    return m_Initialized && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantsCaptureKeyboard() const {
    return m_Initialized && ImGui::GetIO().WantCaptureKeyboard;
}

} // namespace Hockey
