#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

class Config;

struct EditorPanelOpenState {
    std::string name;
    bool open = true;

    friend bool operator==(const EditorPanelOpenState&, const EditorPanelOpenState&) = default;
};

// Persistent editor preferences. Stored as TOML under data/editor and loaded on
// startup / saved on shutdown or whenever a value changes.
struct EditorSettings {
    bool autosaveEnabled = true;
    int autosaveIntervalSeconds = 120;

    bool validateBeforeSave = true;
    bool validateAfterLoad = true;

    bool showGrid = true;
    float gridSpacing = 1.0f;

    bool snapEnabled = false;
    float moveSnap = 0.5f;
    float rotateSnapDegrees = 15.0f;
    float scaleSnap = 0.1f;

    float cameraMoveSpeed = 10.0f;
    float cameraFastMultiplier = 4.0f;
    float cameraMouseSensitivity = 0.15f;

    bool restoreLastScene = true;
    std::vector<std::filesystem::path> recentScenes;
    std::vector<EditorPanelOpenState> panelOpenStates;

    // Content pipeline behaviour (the [assets] section). Drives startup discovery
    // and the per-frame hot-reload poll in EditorApp.
    bool assetsAutoImport = false;
    bool assetsAutoCookDirty = false;
    bool assetsHotReload = false;

    // Maximum number of most-recently-used scene paths retained.
    static constexpr std::size_t kMaxRecentScenes = 10;

    // Default on-disk location: <root>/data/editor/editor_settings.toml.
    static std::filesystem::path DefaultPath();

    // Seeds settings from the project-level config (editor.toml): the
    // [scene] autosave keys and the [assets] section. Intended to run BEFORE
    // Load() so the user's editor_settings.toml still wins where it overrides.
    void ApplyProjectConfig(const Config& projectConfig);

    // Loads settings from a TOML file, leaving defaults for any missing keys.
    // Returns failure only when the file exists but cannot be parsed.
    Status Load(const std::filesystem::path& path);

    // Writes the current settings to a TOML file (creating parent dirs).
    Status Save(const std::filesystem::path& path) const;

    // Records a scene path as the most-recently-used, de-duplicating and
    // trimming to kMaxRecentScenes.
    void AddRecentScene(const std::filesystem::path& path);

    bool PanelOpenOrDefault(std::string_view name, bool defaultOpen) const;
    bool SetPanelOpen(std::string name, bool open);
    bool SetPanelOpenStates(std::vector<EditorPanelOpenState> states);
    void ClearPanelOpenStates();
};

} // namespace Hockey
