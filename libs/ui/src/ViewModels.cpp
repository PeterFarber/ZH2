#include "Hockey/UI/ViewModels.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Hockey {

std::string FormatClockText(float secondsRemaining) {
    const int totalSeconds = std::max(0, static_cast<int>(std::round(secondsRemaining)));
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
    return buffer;
}

std::string FormatShotCharge(float ratio) {
    const int percent = static_cast<int>(std::round(std::clamp(ratio, 0.0f, 1.0f) * 100.0f));
    return std::to_string(percent) + "%";
}

} // namespace Hockey
