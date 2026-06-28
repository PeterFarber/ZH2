#pragma once

#include <filesystem>
#include <string>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class EditorContext;

class GameplayTuningPanel final : public Panel {
public:
    GameplayTuningPanel();
    void OnImGui(EditorContext& context) override;

private:
    enum class Section {
        Tuning,
        EditorSettings,
        ServerSettings,
    };

    void EnsureLoaded(EditorContext& context);
    void LoadAll(EditorContext& context);
    void DrawNavigation();
    void DrawTuning(EditorContext& context);
    void DrawSettings(EditorContext& context, GameplaySettings& settings, const char* scope);
    void SaveTuning(EditorContext& context);
    void SaveEditorConfig(EditorContext& context);
    void SaveServerConfig(EditorContext& context);
    void ApplyToPreview(EditorContext& context);

    Config m_EditorConfig;
    Config m_ServerConfig;
    Config m_DefaultConfig;

    GameplayTuning m_Tuning;
    GameplaySettings m_DefaultSettings;
    GameplaySettings m_EditorSettings;
    GameplaySettings m_ServerSettings;

    std::filesystem::path m_TuningPath;
    std::filesystem::path m_EditorPath;

    std::string m_Status;
    Section m_Section = Section::Tuning;
    bool m_Loaded = false;
    bool m_TuningDirty = false;
    bool m_EditorDirty = false;
    bool m_ServerDirty = false;
};

} // namespace Hockey
