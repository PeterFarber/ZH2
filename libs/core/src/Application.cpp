#include "Hockey/Core/Application.hpp"
#include <utility>
namespace Hockey {
Application::Application(CommandLine commandLine) : m_CommandLine(std::move(commandLine)) {}
int Application::Run() {
    if (!OnInit()) {
        OnShutdown();
        return 1;
    }
    OnShutdown();
    return 0;
}
void Application::RequestQuit() {
    m_Running = false;
}
bool Application::IsRunning() const {
    return m_Running;
}
CommandLine& Application::GetCommandLine() {
    return m_CommandLine;
}
} // namespace Hockey
