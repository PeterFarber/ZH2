#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hockey {

// Cross-platform polling file watcher. Tracks last-write timestamps of files
// under a root directory and reports new/modified/deleted files between polls.
// Polling is used (instead of OS-specific notify APIs) for portability and
// determinism in tests. Metadata sidecars are ignored.
class AssetWatcher {
public:
    void Start(const std::filesystem::path& rawRoot);
    void Stop();
    bool IsRunning() const { return m_Running; }

    // Returns the set of files that were added, modified, or deleted since the
    // previous poll (deleted files are still reported once, then forgotten).
    std::vector<std::filesystem::path> PollChangedFiles();

private:
    bool m_Running = false;
    std::filesystem::path m_Root;
    std::unordered_map<std::string, uint64_t> m_Timestamps;
};

} // namespace Hockey
