#pragma once

#include <string>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"

namespace Hockey {

class Window;
class Renderer;

// Owns the Dear ImGui context plus its SDL3 platform backend and the Vulkan
// render backend (via ImGuiRendererBridge). Drives the per-frame ImGui lifecycle
// (NewFrame/Render) and feeds SDL events into ImGui. The editor relies entirely
// on ImGui's IO for input, so no core EventQueue plumbing is required.
class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    Status Init(Window& window, Renderer& renderer, float editorScale = 1.0f);
    void Shutdown();
    void SaveLayout() const;
    void ApplyEditorScale(float editorScale);

    // Forwards a raw SDL_Event (passed as void* to keep SDL out of editor
    // headers) to the ImGui SDL3 backend. Returns true if ImGui consumed it.
    bool ProcessEvent(const void* sdlEvent);

    // Begins a new ImGui frame (platform + renderer + ImGuizmo). Call once per
    // frame before building UI.
    void BeginFrame();

    // Finalizes the ImGui frame and records its draw data into the renderer's
    // current frame. Call after all UI has been built and a renderer frame is
    // active.
    void EndFrame();

    bool WantsCaptureMouse() const;
    bool WantsCaptureKeyboard() const;

    ImGuiRendererBridge& Bridge() {
        return m_Bridge;
    }

    bool IsInitialized() const {
        return m_Initialized;
    }

    bool IconFontLoaded() const {
        return m_IconFontLoaded;
    }

    const std::string& IniPath() const {
        return m_IniPath;
    }

private:
    void LoadEditorFonts();

    ImGuiRendererBridge m_Bridge;
    std::string m_IniPath; // backing storage for ImGuiIO::IniFilename
    bool m_Initialized = false;
    bool m_PlatformInitialized = false;
    bool m_IconFontLoaded = false;
};

} // namespace Hockey
