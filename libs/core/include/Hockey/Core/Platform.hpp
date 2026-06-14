#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
namespace Hockey {
class Platform {
public:
    static std::filesystem::path ExecutablePath();
    static std::filesystem::path CurrentWorkingDirectory();
    static std::string OSName();
    static uint32_t HardwareThreadCount();
    static void SleepMilliseconds(uint32_t ms);
    static void SleepMicroseconds(uint64_t us);
    static void YieldThread();
    static bool IsDebuggerAttached();
};
} // namespace Hockey
