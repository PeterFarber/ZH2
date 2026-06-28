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
        EditorApplication,
        EditorWindowInput,
        EditorGraphics,
        EditorLightingShadows,
        EditorSceneAutosave,
        EditorAssets,
        EditorPhysicsPreview,
        EditorGameplayPreview,
        ServerApplication,
        ServerSimulation,
        ServerPhysics,
        ServerGameplay,
        ServerStartupScene,
        Preferences,
    };

    void EnsureLoaded(EditorContext& context);
    void LoadAll(EditorContext& context);
    void RefreshDerivedSettings();
    void ResetCurrentSection(EditorContext& context);

    void DrawNavigation();
    void DrawEditorApplication(EditorContext& context);
    void DrawEditorWindowInput(EditorContext& context);
    void DrawEditorGraphics(EditorContext& context);
    void DrawEditorLightingShadows(EditorContext& context);
    void DrawEditorSceneAutosave(EditorContext& context);
    void DrawEditorAssets(EditorContext& context);
    void DrawEditorPhysicsPreview(EditorContext& context);
    void DrawEditorGameplayPreview(EditorContext& context);
    void DrawServerApplication(EditorContext& context);
    void DrawServerSimulation(EditorContext& context);
    void DrawServerPhysics(EditorContext& context);
    void DrawServerGameplay(EditorContext& context);
    void DrawServerStartupScene(EditorContext& context);
    void DrawPreferences(EditorContext& context);

    void SaveEditorConfig(EditorContext& context);
    void SaveServerBuildDefaults(EditorContext& context);
    void SaveUserPreferences(EditorContext& context);

    Config m_EditorConfig;
    Config m_ServerConfig;
    Config m_DefaultConfig;

    RendererSettings m_EditorRenderer;
    RendererSettings m_DefaultRenderer;
    PhysicsSettings m_EditorPhysics;
    PhysicsSettings m_ServerPhysics;
    PhysicsSettings m_DefaultPhysics;
    GameplaySettings m_EditorGameplay;
    GameplaySettings m_ServerGameplay;
    GameplaySettings m_DefaultGameplay;

    std::filesystem::path m_EditorPath;
    std::filesystem::path m_UserPreferencesPath;
    std::string m_Status;

    Section m_Section = Section::EditorApplication;
    bool m_Loaded = false;
    bool m_EditorRestartRequired = false;
    bool m_ServerRestartRequired = false;
};

} // namespace Hockey
