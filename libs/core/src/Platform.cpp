#include "Hockey/Core/Platform.hpp"
#include <chrono>
#include <thread>
#if HK_PLATFORM_WINDOWS
#include <windows.h>
#elif HK_PLATFORM_LINUX
#include <fstream>
#include <limits.h>
#include <string>
#include <unistd.h>
#endif
namespace Hockey {
std::filesystem::path Platform::ExecutablePath() {
#if HK_PLATFORM_WINDOWS
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer);
#elif HK_PLATFORM_LINUX
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len > 0) {
        buffer[len] = '\0';
        return buffer;
    }
    return {};
#endif
}
std::filesystem::path Platform::CurrentWorkingDirectory() {
    return std::filesystem::current_path();
}
std::string Platform::OSName() {
    return HK_PLATFORM_WINDOWS ? "Windows" : "Linux";
}
uint32_t Platform::HardwareThreadCount() {
    return std::thread::hardware_concurrency();
}
void Platform::SleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void Platform::SleepMicroseconds(uint64_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}
void Platform::YieldThread() {
    std::this_thread::yield();
}
bool Platform::IsDebuggerAttached() {
#if HK_PLATFORM_WINDOWS
    return IsDebuggerPresent() != 0;
#elif HK_PLATFORM_LINUX
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("TracerPid:", 0) == 0) {
            return line.find_first_of("123456789", 10) != std::string::npos;
        }
    }
    return false;
#else
    return false;
#endif
}
} // namespace Hockey
