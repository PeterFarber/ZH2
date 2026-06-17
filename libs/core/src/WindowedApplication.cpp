#include "Hockey/Core/WindowedApplication.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/SignalHandler.hpp"
#include "Hockey/Core/Time.hpp"
#include "Hockey/Core/Timer.hpp"
#include <SDL3/SDL.h>
namespace Hockey {
bool WindowedApplication::CreateAppWindowFromConfig(const Config& config) {
    WindowDesc desc;
    desc.title = config.GetString("window.title", desc.title);
    desc.width = static_cast<uint32_t>(config.GetInt("window.width", static_cast<int>(desc.width)));
    desc.height = static_cast<uint32_t>(config.GetInt("window.height", static_cast<int>(desc.height)));
    desc.resizable = config.GetBool("window.resizable", desc.resizable);
    desc.maximized = config.GetBool("window.maximized", desc.maximized);
    desc.startCentered = config.GetBool("window.start_centered", desc.startCentered);
    desc.vulkan = config.GetBool("window.vulkan", true);
    SetTargetFps(config.GetInt("app.target_fps", m_TargetFps));
    return m_Window.Create(desc);
}
Window& WindowedApplication::GetWindow() {
    return m_Window;
}
void WindowedApplication::SetTargetFps(int fps) {
    m_TargetFps = fps > 0 ? fps : 0;
}
int WindowedApplication::Run() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        return 1;
    }
    // Hook Ctrl+C / SIGTERM (and the Windows console close events) so the
    // windowed client/editor shut down cleanly when signalled, matching the
    // headless server's behaviour.
    SignalHandler::Install();
    Input::Init();
    if (!OnInit()) {
        OnShutdown();
        Input::Shutdown();
        SDL_Quit();
        return 1;
    }
    // Command-line --fps-limit overrides the config-provided cap.
    if (GetCommandLine().Has("--fps-limit")) {
        SetTargetFps(GetCommandLine().GetInt("--fps-limit", m_TargetFps));
    }
    m_MaxFrames = GetCommandLine().GetInt("--max-frames", GetCommandLine().GetInt("--frames", 0));
    if (m_MaxFrames < 0) {
        m_MaxFrames = 0;
    }
    int frameCount = 0;
    Timer timer;
    while (IsRunning() && !m_Window.ShouldClose() && !SignalHandler::ShutdownRequested()) {
        const double frameStart = Time::NowSeconds();
        const float deltaTime = static_cast<float>(timer.ElapsedSeconds());
        timer.Reset();

        Input::BeginFrame();
        m_Window.PollEvents(m_EventQueue);
        Event event;
        while (m_EventQueue.Poll(event)) {
            Input::ProcessEvent(event);
            OnEvent(event);
            if (event.type == EventType::WindowClose) {
                RequestQuit();
            }
        }
        OnUpdate(deltaTime);
        Input::EndFrame();

        if (m_TargetFps > 0) {
            const double targetFrameSeconds = 1.0 / static_cast<double>(m_TargetFps);
            const double remaining = targetFrameSeconds - (Time::NowSeconds() - frameStart);
            if (remaining > 0.0) {
                Platform::SleepMicroseconds(static_cast<uint64_t>(remaining * 1'000'000.0));
            }
        }
        if (m_MaxFrames > 0 && ++frameCount >= m_MaxFrames) {
            RequestQuit();
        }
    }
    OnShutdown();
    Input::Shutdown();
    SDL_Quit();
    return 0;
}
} // namespace Hockey
