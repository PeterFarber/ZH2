#include "Hockey/Core/Log.hpp"
#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>
namespace Hockey {
static std::shared_ptr<spdlog::logger> g_Core, g_Client, g_Editor, g_Server, g_Tests;
namespace {
std::shared_ptr<spdlog::logger> MakeLogger(const std::string& name, const std::vector<spdlog::sink_ptr>& sinks) {
    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    // Timestamp, logger name, level, thread id, message.
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::warn);
    spdlog::register_logger(logger);
    return logger;
}
} // namespace
bool Log::Init(const std::filesystem::path& logFile) {
    if (g_Core) {
        return true;
    }
    if (logFile.has_parent_path()) {
        std::filesystem::create_directories(logFile.parent_path());
    }
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true);
    std::vector<spdlog::sink_ptr> sinks{console, file};
    g_Core = MakeLogger("Core", sinks);
    g_Client = MakeLogger("Client", sinks);
    g_Editor = MakeLogger("Editor", sinks);
    g_Server = MakeLogger("Server", sinks);
    g_Tests = MakeLogger("Tests", sinks);
    return true;
}
void Log::Shutdown() {
    spdlog::shutdown();
    g_Core.reset();
    g_Client.reset();
    g_Editor.reset();
    g_Server.reset();
    g_Tests.reset();
}
std::shared_ptr<spdlog::logger>& Log::Core() {
    return g_Core;
}
std::shared_ptr<spdlog::logger>& Log::Client() {
    return g_Client;
}
std::shared_ptr<spdlog::logger>& Log::Editor() {
    return g_Editor;
}
std::shared_ptr<spdlog::logger>& Log::Server() {
    return g_Server;
}
std::shared_ptr<spdlog::logger>& Log::Tests() {
    return g_Tests;
}
} // namespace Hockey
