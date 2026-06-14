#pragma once
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
namespace Hockey {
class Log {
public:
    static bool Init(const std::filesystem::path& logFile);
    static void Shutdown();
    static std::shared_ptr<spdlog::logger>& Core();
    static std::shared_ptr<spdlog::logger>& Client();
    static std::shared_ptr<spdlog::logger>& Editor();
    static std::shared_ptr<spdlog::logger>& Server();
    static std::shared_ptr<spdlog::logger>& Tests();
};
} // namespace Hockey
#define HK_CORE_TRACE(...) ::Hockey::Log::Core()->trace(__VA_ARGS__)
#define HK_CORE_INFO(...) ::Hockey::Log::Core()->info(__VA_ARGS__)
#define HK_CORE_WARN(...) ::Hockey::Log::Core()->warn(__VA_ARGS__)
#define HK_CORE_ERROR(...) ::Hockey::Log::Core()->error(__VA_ARGS__)
#define HK_CORE_CRITICAL(...) ::Hockey::Log::Core()->critical(__VA_ARGS__)
#define HK_CLIENT_INFO(...) ::Hockey::Log::Client()->info(__VA_ARGS__)
#define HK_EDITOR_TRACE(...) ::Hockey::Log::Editor()->trace(__VA_ARGS__)
#define HK_EDITOR_INFO(...) ::Hockey::Log::Editor()->info(__VA_ARGS__)
#define HK_EDITOR_WARN(...) ::Hockey::Log::Editor()->warn(__VA_ARGS__)
#define HK_EDITOR_ERROR(...) ::Hockey::Log::Editor()->error(__VA_ARGS__)
#define HK_EDITOR_CRITICAL(...) ::Hockey::Log::Editor()->critical(__VA_ARGS__)
#define HK_SERVER_INFO(...) ::Hockey::Log::Server()->info(__VA_ARGS__)
#define HK_TEST_INFO(...) ::Hockey::Log::Tests()->info(__VA_ARGS__)
