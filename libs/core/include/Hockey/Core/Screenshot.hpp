#pragma once

#include <filesystem>
#include <string_view>

namespace Hockey {

// Returns a unique PNG path under <project>/_save/screenshots.
std::filesystem::path MakeScreenshotPath(std::string_view prefix);

} // namespace Hockey
