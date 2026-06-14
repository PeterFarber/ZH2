#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace Hockey {

// One captured log line for the Console panel.
struct EditorLogEntry {
    std::string message;
    std::string loggerName;
    std::string level;     // short tag: "trace"/"debug"/"info"/"warn"/"error"/"crit"
    std::string timestamp; // HH:MM:SS.mmm (local time)
    std::string sourceFile;
    int sourceLine = 0;
    int levelValue = 0; // spdlog level int, for filtering
};

// Thread-safe ring buffer of recent log entries. A spdlog sink (attached to the
// engine loggers via InstallEditorConsoleSink) feeds it from any thread; the
// Console panel reads snapshots on the UI thread. A monotonic revision counter
// lets the panel cheaply detect changes and avoid copying every frame.
class EditorConsoleStore {
public:
    static EditorConsoleStore& Get();

    void Push(EditorLogEntry entry);
    void Clear();

    std::size_t Revision() const;
    std::vector<EditorLogEntry> Snapshot() const;

    void SetCapacity(std::size_t capacity);

private:
    EditorConsoleStore() = default;

    mutable std::mutex m_Mutex;
    std::vector<EditorLogEntry> m_Entries;
    std::size_t m_Capacity = 5000;
    std::size_t m_Revision = 0;
};

// Attaches the editor console sink to all engine loggers. Idempotent and safe to
// call after Log::Init. No-op if logging has not been initialised.
void InstallEditorConsoleSink();

} // namespace Hockey
