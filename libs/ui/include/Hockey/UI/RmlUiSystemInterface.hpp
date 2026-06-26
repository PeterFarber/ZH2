#pragma once

#include <chrono>

#include <RmlUi/Core/SystemInterface.h>

namespace Hockey {

class RmlUiSystemInterface final : public Rml::SystemInterface {
public:
    RmlUiSystemInterface();

    double GetElapsedTime() override;
    void JoinPath(Rml::String& translatedPath, const Rml::String& documentPath, const Rml::String& path) override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

private:
    std::chrono::steady_clock::time_point m_StartTime;
};

} // namespace Hockey
