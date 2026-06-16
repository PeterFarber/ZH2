#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetWatcher.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <fstream>
#include <thread>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {
void WriteFile(const fs::path& path, const std::string& contents) {
    FileSystem::CreateDirectories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << contents;
}
} // namespace

void RunHotReloadTests() {
    HockeyTest::BeginSuite("HotReloadTests");

    // Use an isolated temp workspace as a mini project root.
    const fs::path workspace = Paths::TempFile("hotreload_ws");
    FileSystem::Remove(workspace);
    const fs::path rawRoot = workspace / "data" / "raw";
    FileSystem::CreateDirectories(rawRoot / "shaders");

    const fs::path watched = rawRoot / "shaders" / "watch_test.frag";
    WriteFile(watched, "void main() {}\n");

    AssetWatcher watcher;
    watcher.Start(rawRoot);
    // No changes immediately after the baseline snapshot.
    HK_CHECK_EQ(static_cast<int>(watcher.PollChangedFiles().size()), 0);

    // Modify the file; the watcher should detect it. Bump the mtime explicitly
    // so the change is visible regardless of filesystem timestamp resolution.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    WriteFile(watched, "void main() { /* changed */ }\n");
    fs::last_write_time(watched, fs::file_time_type::clock::now());
    const auto changed = watcher.PollChangedFiles();
    HK_CHECK_MSG(!changed.empty(), "watcher detects modified file");

    // Delete detection.
    FileSystem::Remove(watched);
    const auto deleted = watcher.PollChangedFiles();
    HK_CHECK_MSG(!deleted.empty(), "watcher detects deleted file");
    watcher.Stop();
    HK_CHECK_MSG(!watcher.IsRunning(), "watcher stops");

    // Manager-level hot reload: changing a discovered shader marks it dirty and
    // emits a Modified event.
    WriteFile(watched, "void main() {}\n");
    AssetManager manager;
    AssetManagerCreateInfo info = AssetManager::DefaultCreateInfo(workspace);
    HK_CHECK_MSG(static_cast<bool>(manager.Init(info)), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "discover ok");

    AssetMetadata* shader = manager.Database().FindByRawPath("data/raw/shaders/watch_test.frag");
    HK_CHECK_MSG(shader != nullptr, "shader discovered");

    manager.StartWatching();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    WriteFile(watched, "void main() { /* v2 */ }\n");
    fs::last_write_time(watched, fs::file_time_type::clock::now());
    const int count = manager.PollHotReload(/*autoImport=*/false, /*autoCookDirty=*/false);
    HK_CHECK_MSG(count > 0, "manager poll detects change");

    bool sawModified = false;
    for (const AssetEvent& event : manager.PollEvents()) {
        if (event.type == AssetEventType::Modified) {
            sawModified = true;
        }
    }
    HK_CHECK_MSG(sawModified, "modified event emitted");
    shader = manager.Database().FindByRawPath("data/raw/shaders/watch_test.frag");
    HK_CHECK_MSG(shader != nullptr && shader->dirty, "asset marked dirty after change");

    // Move detection: renaming the raw file (identical content) should keep the
    // asset id and surface a single Moved event instead of Deleted + Imported.
    const AssetID movedFromId = shader->id;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const fs::path renamed = rawRoot / "shaders" / "watch_test_renamed.frag";
    fs::rename(watched, renamed);
    fs::last_write_time(renamed, fs::file_time_type::clock::now());
    const int moveCount = manager.PollHotReload(/*autoImport=*/false, /*autoCookDirty=*/false);
    HK_CHECK_MSG(moveCount >= 2, "poll observes both sides of the rename");

    bool sawMoved = false;
    AssetID movedId;
    for (const AssetEvent& event : manager.PollEvents()) {
        if (event.type == AssetEventType::Moved) {
            sawMoved = true;
            movedId = event.id;
        }
    }
    HK_CHECK_MSG(sawMoved, "moved event emitted on rename");
    HK_CHECK_MSG(movedId == movedFromId, "moved asset keeps its id");
    HK_CHECK_MSG(manager.Database().FindByRawPath("data/raw/shaders/watch_test_renamed.frag") != nullptr,
                 "database repointed to the new path");
    HK_CHECK_MSG(manager.Database().FindByRawPath("data/raw/shaders/watch_test.frag") == nullptr,
                 "old path dropped from the database");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
