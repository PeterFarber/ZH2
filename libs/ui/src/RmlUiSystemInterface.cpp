#include "Hockey/UI/RmlUiSystemInterface.hpp"

#include <filesystem>

#include "Hockey/Core/Log.hpp"

namespace Hockey {

RmlUiSystemInterface::RmlUiSystemInterface() : m_StartTime(std::chrono::steady_clock::now()) {}

double RmlUiSystemInterface::GetElapsedTime() {
    const auto elapsed = std::chrono::steady_clock::now() - m_StartTime;
    return std::chrono::duration<double>(elapsed).count();
}

void RmlUiSystemInterface::JoinPath(Rml::String& translatedPath, const Rml::String& documentPath,
                                    const Rml::String& path) {
    std::filesystem::path resourcePath(path);
    if (resourcePath.is_absolute() || resourcePath.has_root_name() || resourcePath.has_root_directory()) {
        translatedPath = resourcePath.lexically_normal().generic_string();
        return;
    }

    const std::filesystem::path documentDirectory = std::filesystem::path(documentPath).parent_path();
    translatedPath = (documentDirectory / resourcePath).lexically_normal().generic_string();
}

bool RmlUiSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message) {
    auto& logger = Log::Core();
    if (!logger) {
        return true;
    }

    switch (type) {
    case Rml::Log::LT_ALWAYS:
    case Rml::Log::LT_INFO:
        logger->info("RmlUi: {}", message);
        break;
    case Rml::Log::LT_WARNING:
        logger->warn("RmlUi: {}", message);
        break;
    case Rml::Log::LT_ERROR:
    case Rml::Log::LT_ASSERT:
        logger->error("RmlUi: {}", message);
        break;
    case Rml::Log::LT_DEBUG:
    case Rml::Log::LT_MAX:
    default:
        logger->debug("RmlUi: {}", message);
        break;
    }

    return true;
}

} // namespace Hockey
