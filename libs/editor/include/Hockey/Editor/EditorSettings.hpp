#pragma once

#include <filesystem>
#include <vector>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

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

    // Content pipeline behaviour (the [assets] section). Drives startup discovery
    // and the per-frame hot-reload poll in EditorApp.
    bool assetsAutoDiscover = true;
    bool assetsAutoImport = true;
    bool assetsAutoCookDirty = true;
    bool assetsHotReload = true;

    // Maximum number of most-recently-used scene paths retained.
    static constexpr std::size_t kMaxRecentScenes = 10;

    // Default on-disk location: <root>/data/editor/editor_settings.toml.
    static std::filesystem::path DefaultPath();

    // Loads settings from a TOML file, leaving defaults for any missing keys.
    // Returns failure only when the file exists but cannot be parsed.
    Status Load(const std::filesystem::path& path);

    // Writes the current settings to a TOML file (creating parent dirs).
    Status Save(const std::filesystem::path& path) const;

    // Records a scene path as the most-recently-used, de-duplicating and
    // trimming to kMaxRecentScenes.
    void AddRecentScene(const std::filesystem::path& path);
};

} // namespace Hockey
