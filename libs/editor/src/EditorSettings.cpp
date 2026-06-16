#include "Hockey/Editor/EditorSettings.hpp"

#include <algorithm>
#include <string>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"

namespace Hockey {

std::filesystem::path EditorSettings::DefaultPath() {
    return Paths::DataFile("editor/editor_settings.toml");
}

void EditorSettings::ApplyProjectConfig(const Config& projectConfig) {
    // editor.toml [scene]
    autosaveEnabled = projectConfig.GetBool("scene.autosave_enabled", autosaveEnabled);
    autosaveIntervalSeconds = projectConfig.GetInt("scene.autosave_interval_seconds", autosaveIntervalSeconds);

    // editor.toml [assets]
    assetsAutoDiscover = projectConfig.GetBool("assets.auto_discover", assetsAutoDiscover);
    assetsAutoImport = projectConfig.GetBool("assets.auto_import", assetsAutoImport);
    assetsAutoCookDirty = projectConfig.GetBool("assets.auto_cook_dirty", assetsAutoCookDirty);
    assetsHotReload = projectConfig.GetBool("assets.hot_reload", assetsHotReload);
}

Status EditorSettings::Load(const std::filesystem::path& path) {
    Config config;
    if (std::filesystem::exists(path)) {
        if (const Status status = config.Load(path); !status) {
            return status;
        }
    }

    autosaveEnabled = config.GetBool("autosave.enabled", autosaveEnabled);
    autosaveIntervalSeconds = config.GetInt("autosave.interval_seconds", autosaveIntervalSeconds);

    validateBeforeSave = config.GetBool("validation.before_save", validateBeforeSave);
    validateAfterLoad = config.GetBool("validation.after_load", validateAfterLoad);

    showGrid = config.GetBool("grid.show", showGrid);
    gridSpacing = static_cast<float>(config.GetDouble("grid.spacing", gridSpacing));

    snapEnabled = config.GetBool("snap.enabled", snapEnabled);
    moveSnap = static_cast<float>(config.GetDouble("snap.move", moveSnap));
    rotateSnapDegrees = static_cast<float>(config.GetDouble("snap.rotate_degrees", rotateSnapDegrees));
    scaleSnap = static_cast<float>(config.GetDouble("snap.scale", scaleSnap));

    cameraMoveSpeed = static_cast<float>(config.GetDouble("camera.move_speed", cameraMoveSpeed));
    cameraFastMultiplier = static_cast<float>(config.GetDouble("camera.fast_multiplier", cameraFastMultiplier));
    cameraMouseSensitivity = static_cast<float>(config.GetDouble("camera.mouse_sensitivity", cameraMouseSensitivity));

    restoreLastScene = config.GetBool("scene.restore_last", restoreLastScene);

    assetsAutoDiscover = config.GetBool("assets.auto_discover", assetsAutoDiscover);
    assetsAutoImport = config.GetBool("assets.auto_import", assetsAutoImport);
    assetsAutoCookDirty = config.GetBool("assets.auto_cook_dirty", assetsAutoCookDirty);
    assetsHotReload = config.GetBool("assets.hot_reload", assetsHotReload);

    recentScenes.clear();
    const int recentCount = config.GetInt("scene.recent_count", 0);
    for (int i = 0; i < recentCount; ++i) {
        const std::string key = "scene.recent_" + std::to_string(i);
        const std::string value = config.GetString(key, "");
        if (!value.empty()) {
            recentScenes.emplace_back(value);
        }
    }

    return Status::Ok();
}

Status EditorSettings::Save(const std::filesystem::path& path) const {
    Config config;

    config.SetBool("autosave.enabled", autosaveEnabled);
    config.SetInt("autosave.interval_seconds", autosaveIntervalSeconds);

    config.SetBool("validation.before_save", validateBeforeSave);
    config.SetBool("validation.after_load", validateAfterLoad);

    config.SetBool("grid.show", showGrid);
    config.SetDouble("grid.spacing", gridSpacing);

    config.SetBool("snap.enabled", snapEnabled);
    config.SetDouble("snap.move", moveSnap);
    config.SetDouble("snap.rotate_degrees", rotateSnapDegrees);
    config.SetDouble("snap.scale", scaleSnap);

    config.SetDouble("camera.move_speed", cameraMoveSpeed);
    config.SetDouble("camera.fast_multiplier", cameraFastMultiplier);
    config.SetDouble("camera.mouse_sensitivity", cameraMouseSensitivity);

    config.SetBool("scene.restore_last", restoreLastScene);

    config.SetBool("assets.auto_discover", assetsAutoDiscover);
    config.SetBool("assets.auto_import", assetsAutoImport);
    config.SetBool("assets.auto_cook_dirty", assetsAutoCookDirty);
    config.SetBool("assets.hot_reload", assetsHotReload);

    const int recentCount = static_cast<int>(std::min(recentScenes.size(), kMaxRecentScenes));
    config.SetInt("scene.recent_count", recentCount);
    for (int i = 0; i < recentCount; ++i) {
        const std::string key = "scene.recent_" + std::to_string(i);
        config.SetString(key, recentScenes[static_cast<std::size_t>(i)].generic_string());
    }

    return config.Save(path);
}

void EditorSettings::AddRecentScene(const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }
    const std::filesystem::path normalized = path.lexically_normal();
    recentScenes.erase(std::remove_if(recentScenes.begin(), recentScenes.end(),
                                      [&](const std::filesystem::path& existing) {
                                          return existing.lexically_normal() == normalized;
                                      }),
                       recentScenes.end());
    recentScenes.insert(recentScenes.begin(), normalized);
    if (recentScenes.size() > kMaxRecentScenes) {
        recentScenes.resize(kMaxRecentScenes);
    }
}

} // namespace Hockey
