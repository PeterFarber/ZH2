#pragma once

#include <string>

namespace Hockey {

class Config;

struct UISettings {
    bool enabled = true;
    float uiScale = 1.0f;
    bool debuggerEnabled = false;
    bool reloadDocuments = false;
    std::string defaultFont = "data/ui/fonts/Inter-Regular.ttf";
    std::string themeDocument = "data/ui/themes/hockey.rcss";
    std::string startFlow = "data/ui/flows/default.clientflow.yaml";
};

UISettings MakeDefaultUISettings();
UISettings LoadUISettings(const Config& config);
void SaveUISettings(Config& config, const UISettings& settings);

} // namespace Hockey
