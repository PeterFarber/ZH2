#include "Hockey/Core/Time.hpp"
#include <chrono>
namespace Hockey {
double Time::NowSeconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}
uint64_t Time::NowNanoseconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}
}
