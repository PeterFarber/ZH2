#include "Hockey/Core/Screenshot.hpp"

#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

#include "Hockey/Core/Paths.hpp"

namespace Hockey {
namespace fs = std::filesystem;

namespace {

std::string SanitizePrefix(std::string_view prefix) {
    std::string out;
    out.reserve(prefix.size());
    for (const char ch : prefix) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c)) {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else if (ch == '-' || ch == '_') {
            out.push_back(ch);
        } else if (!out.empty() && out.back() != '_') {
            out.push_back('_');
        }
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out.empty() ? std::string("screenshot") : out;
}

std::tm LocalTime(std::time_t value) {
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &value);
#else
    localtime_r(&value, &tm);
#endif
    return tm;
}

} // namespace

fs::path MakeScreenshotPath(std::string_view prefix) {
    const fs::path dir = Paths::Get().root / "_save" / "screenshots";
    std::error_code ec;
    fs::create_directories(dir, ec);

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = LocalTime(time);

    std::ostringstream name;
    name << SanitizePrefix(prefix) << '_' << std::put_time(&tm, "%Y%m%d_%H%M%S");
    const std::string base = name.str();

    fs::path candidate = dir / (base + ".png");
    for (int i = 1; fs::exists(candidate, ec) && i < 10000; ++i) {
        candidate = dir / (base + '_' + std::to_string(i) + ".png");
    }
    return candidate;
}

} // namespace Hockey
