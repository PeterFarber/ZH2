#pragma once

#include <filesystem>
#include <string>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"

namespace Hockey {

// Unity-style Project Settings window. Edits project-level app configs under
// data/config plus the editor's user preferences in data/editor.
class ProjectSettingsPanel : public Panel {
public:
    ProjectSettingsPanel();
    void OnImGui(EditorContext& context) override;

private:
    enum class Section {
        EditorGeneral,
        EditorGraphics,
        EditorPhysics,
        EditorGameplay,
        ClientGeneral,
        ClientGraphics,
        ClientPhysics,
        ClientGameplay,
        ServerGeneral,
        ServerPhysics,
        ServerGameplay,
        Preferences,
    };

    void EnsureLoaded(EditorContext& context);
    void LoadAll(EditorContext& context);
    void RefreshDerivedSettings();

    void DrawNavigation();
    void DrawEditorGeneral(EditorContext& context);
    void DrawEditorGraphics(EditorContext& context);
    void DrawEditorPhysics(EditorContext& context);
    void DrawEditorGameplay(EditorContext& context);
    void DrawClientGeneral();
    void DrawClientGraphics();
    void DrawClientPhysics();
    void DrawClientGameplay();
    void DrawServerGeneral();
    void DrawServerPhysics();
    void DrawServerGameplay();
    void DrawPreferences(EditorContext& context);

    void SaveEditorConfig(EditorContext& context);
    void SaveClientConfig();
    void SaveServerConfig();
    void SaveUserPreferences(EditorContext& context);

    Config m_EditorConfig;
    Config m_ClientConfig;
    Config m_ServerConfig;

    RendererSettings m_EditorRenderer;
    RendererSettings m_ClientRenderer;
    PhysicsSettings m_EditorPhysics;
    PhysicsSettings m_ClientPhysics;
    PhysicsSettings m_ServerPhysics;
    GameplaySettings m_EditorGameplay;
    GameplaySettings m_ClientGameplay;
    GameplaySettings m_ServerGameplay;

    std::filesystem::path m_EditorPath;
    std::filesystem::path m_ClientPath;
    std::filesystem::path m_ServerPath;
    std::filesystem::path m_UserPreferencesPath;
    std::string m_Status;

    Section m_Section = Section::EditorGeneral;
    bool m_Loaded = false;
    bool m_EditorRestartRequired = false;
    bool m_ClientRestartRequired = false;
    bool m_ServerRestartRequired = false;
};

} // namespace Hockey
