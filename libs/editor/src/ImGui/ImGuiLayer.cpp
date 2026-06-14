#include "Hockey/Editor/ImGui/ImGuiLayer.hpp"

#include <filesystem>
#include <system_error>

#include <imgui.h>

#include <imgui_impl_sdl3.h>

#include <ImGuizmo.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Window.hpp"
#include "Hockey/Editor/ImGui/EditorTheme.hpp"

namespace Hockey {

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

Status ImGuiLayer::Init(Window& window, Renderer& renderer) {
    if (m_Initialized) {
        return Status::Ok();
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Persist the docking layout alongside the editor settings (data/editor).
    const std::filesystem::path layoutPath = Paths::DataFile("editor/layout.ini");
    std::error_code ec;
    std::filesystem::create_directories(layoutPath.parent_path(), ec);
    m_IniPath = layoutPath.string();
    io.IniFilename = m_IniPath.c_str();

    EditorTheme::ApplyDark();

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
