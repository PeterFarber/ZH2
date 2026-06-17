#pragma once
#include "Hockey/Core/Application.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Window.hpp"
namespace Hockey {
class WindowedApplication : public Application {
public:
    using Application::Application;
    int Run() override;

protected:
    bool CreateAppWindowFromConfig(const Config& config);
    Window& GetWindow();

    // 0 means uncapped. Overridden by --fps-limit on the command line.
    void SetTargetFps(int fps);

    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnEvent(const Event&) {}

private:
    Window m_Window;
    EventQueue m_EventQueue;
    int m_TargetFps = 0;
    int m_MaxFrames = 0;
};
} // namespace Hockey
