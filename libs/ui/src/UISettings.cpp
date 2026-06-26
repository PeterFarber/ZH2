#include "Hockey/UI/UISettings.hpp"

#include <algorithm>

#include "Hockey/Core/Config.hpp"

namespace Hockey {

UISettings MakeDefaultUISettings() {
    return UISettings{};
}

UISettings LoadUISettings(const Config& config) {
    UISettings settings = MakeDefaultUISettings();
    settings.enabled = config.GetBool("ui.enabled", settings.enabled);
    settings.uiScale = static_cast<float>(config.GetDouble("ui.ui_scale", settings.uiScale));
    settings.uiScale = std::clamp(settings.uiScale, 0.5f, 3.0f);
    settings.debuggerEnabled = config.GetBool("ui.debugger_enabled", settings.debuggerEnabled);
    settings.reloadDocuments = config.GetBool("ui.reload_documents", settings.reloadDocuments);
    settings.defaultFont = config.GetString("ui.default_font", settings.defaultFont);
    settings.themeDocument = config.GetString("ui.theme_document", settings.themeDocument);
    settings.startFlow = config.GetString("ui.start_flow", settings.startFlow);
    return settings;
}

void SaveUISettings(Config& config, const UISettings& settings) {
    config.SetBool("ui.enabled", settings.enabled);
    config.SetDouble("ui.ui_scale", settings.uiScale);
    config.SetBool("ui.debugger_enabled", settings.debuggerEnabled);
    config.SetBool("ui.reload_documents", settings.reloadDocuments);
    config.SetString("ui.default_font", settings.defaultFont);
    config.SetString("ui.theme_document", settings.themeDocument);
    config.SetString("ui.start_flow", settings.startFlow);
}

} // namespace Hockey
