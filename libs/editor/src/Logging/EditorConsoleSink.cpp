#include "Hockey/Editor/Logging/EditorConsoleSink.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>

#include <spdlog/sinks/base_sink.h>

#include "Hockey/Core/Log.hpp"

namespace Hockey {

namespace {

const char* ShortLevel(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::trace:
        return "trace";
    case spdlog::level::debug:
        return "debug";
    case spdlog::level::info:
        return "info";
    case spdlog::level::warn:
        return "warn";
    case spdlog::level::err:
        return "error";
    case spdlog::level::critical:
        return "crit";
    default:
        return "off";
    }
}

std::string FormatLocalTime(std::chrono::system_clock::time_point timePoint) {
    using namespace std::chrono;
    const std::time_t asTime = system_clock::to_time_t(timePoint);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &asTime);
#else
    localtime_r(&asTime, &local);
#endif
    const auto millis = duration_cast<milliseconds>(timePoint.time_since_epoch()) % 1000;
    std::array<char, 16> hms{};
    std::strftime(hms.data(), hms.size(), "%H:%M:%S", &local);
    std::array<char, 24> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s.%03d", hms.data(), static_cast<int>(millis.count()));
    return std::string(buffer.data());
}

// spdlog sink that converts each formatted message into an EditorLogEntry. We
// intentionally ignore the sink's own formatter and capture structured fields.
class ConsoleStoreSink : public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        EditorLogEntry entry;
        entry.message.assign(msg.payload.data(), msg.payload.size());
        entry.loggerName.assign(msg.logger_name.data(), msg.logger_name.size());
        entry.level = ShortLevel(msg.level);
        entry.levelValue = static_cast<int>(msg.level);
        entry.timestamp = FormatLocalTime(msg.time);
        if (msg.source.filename != nullptr) {
            entry.sourceFile = msg.source.filename;
            entry.sourceLine = msg.source.line;
        }
        EditorConsoleStore::Get().Push(std::move(entry));
    }

    void flush_() override {}
};

} // namespace

EditorConsoleStore& EditorConsoleStore::Get() {
    static EditorConsoleStore store;
    return store;
}

void EditorConsoleStore::Push(EditorLogEntry entry) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Entries.size() >= m_Capacity) {
        // Drop the oldest 10% in one shot to amortise the erase cost.
        const std::size_t drop = (m_Capacity / 10) + 1;
        m_Entries.erase(m_Entries.begin(), m_Entries.begin() + static_cast<std::ptrdiff_t>(drop));
    }
    m_Entries.push_back(std::move(entry));
    ++m_Revision;
}

void EditorConsoleStore::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.clear();
    ++m_Revision;
}

std::size_t EditorConsoleStore::Revision() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Revision;
}

std::vector<EditorLogEntry> EditorConsoleStore::Snapshot() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Entries;
}

void EditorConsoleStore::SetCapacity(std::size_t capacity) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Capacity = capacity > 0 ? capacity : 1;
}

void InstallEditorConsoleSink() {
    static bool installed = false;
    if (installed) {
        return;
    }
    auto sink = std::make_shared<ConsoleStoreSink>();
    sink->set_level(spdlog::level::trace);

    const std::array<std::shared_ptr<spdlog::logger>*, 5> loggers{&Log::Core(), &Log::Client(), &Log::Editor(),
                                                                  &Log::Server(), &Log::Tests()};
    bool attached = false;
    for (auto* logger : loggers) {
        if (*logger) {
            (*logger)->sinks().push_back(sink);
            attached = true;
        }
    }
    installed = attached;
}

} // namespace Hockey
