#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Hockey {

// Runs a single asset-tool command against the project rooted at `root`.
// Returns a process exit code (0 == success). Implemented in Step 13.
int RunAssetToolCommand(const std::filesystem::path& root, const std::string& command,
                        const std::vector<std::string>& args);

void PrintAssetToolUsage();

} // namespace Hockey
