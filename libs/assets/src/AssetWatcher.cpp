#include "Hockey/Assets/AssetWatcher.hpp"

#include "Hockey/Assets/AssetPath.hpp"

#include <system_error>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
uint64_t WriteTimestamp(const fs::path& path) {
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    if (ec) {
        return 0;
    }
    return static_cast<uint64_t>(time.time_since_epoch().count());
}
} // namespace

void AssetWatcher::Start(const fs::path& rawRoot) {
    m_Root = rawRoot;
    m_Timestamps.clear();
    m_Running = true;
    // Seed the baseline so the first poll only reports subsequent changes.
    std::error_code ec;
    if (fs::exists(m_Root, ec) && fs::is_directory(m_Root, ec)) {
        for (auto it = fs::recursive_directory_iterator(m_Root, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const fs::path& path = it->path();
            if (AssetPath::IsMetadataSidecar(path)) {
                continue;
            }
            m_Timestamps[path.generic_string()] = WriteTimestamp(path);
        }
    }
}

void AssetWatcher::Stop() {
    m_Running = false;
    m_Timestamps.clear();
}

std::vector<fs::path> AssetWatcher::PollChangedFiles() {
    std::vector<fs::path> changed;
    if (!m_Running) {
        return changed;
    }

    std::error_code ec;
    std::unordered_map<std::string, uint64_t> current;
    if (fs::exists(m_Root, ec) && fs::is_directory(m_Root, ec)) {
        for (auto it = fs::recursive_directory_iterator(m_Root, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const fs::path& path = it->path();
            if (AssetPath::IsMetadataSidecar(path)) {
                continue;
            }
            const std::string key = path.generic_string();
            const uint64_t stamp = WriteTimestamp(path);
            current[key] = stamp;

            auto prev = m_Timestamps.find(key);
            if (prev == m_Timestamps.end() || prev->second != stamp) {
                changed.push_back(path); // new or modified
            }
        }
    }

    // Files present last time but not now were deleted.
    for (const auto& [key, stamp] : m_Timestamps) {
        if (current.find(key) == current.end()) {
            changed.emplace_back(key);
        }
    }

    m_Timestamps = std::move(current);
    return changed;
}

} // namespace Hockey
