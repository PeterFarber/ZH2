#include "Hockey/Core/SignalHandler.hpp"
#include <atomic>
#if HK_PLATFORM_WINDOWS
#include <windows.h>
#elif HK_PLATFORM_LINUX
#include <csignal>
#endif
namespace Hockey {
namespace {
std::atomic_bool g_ShutdownRequested{false};

#if HK_PLATFORM_WINDOWS
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_ShutdownRequested.store(true);
            return TRUE;
        default:
            return FALSE;
    }
}
#elif HK_PLATFORM_LINUX
void HandlePosixSignal(int) { g_ShutdownRequested.store(true); }
#endif
}
void SignalHandler::Install() {
#if HK_PLATFORM_WINDOWS
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#elif HK_PLATFORM_LINUX
    struct sigaction action{};
    action.sa_handler = &HandlePosixSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
#endif
}
bool SignalHandler::ShutdownRequested() { return g_ShutdownRequested.load(); }
void SignalHandler::Reset() { g_ShutdownRequested.store(false); }
}
