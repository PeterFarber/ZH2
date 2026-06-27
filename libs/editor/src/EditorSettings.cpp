#include "Hockey/Editor/EditorSettings.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"

namespace Hockey {

std::filesystem::path EditorSettings::DefaultPath() {
    return Paths::DataFile("editor/editor_settings.toml");
}

float EditorSettings::NormalizeEditorScale(float value) {
    return std::clamp(value, kMinEditorScale, kMaxEditorScale);
}

void EditorSettings::ApplyProjectConfig(const Config& projectConfig) {
    // editor.toml [scene]
    autosaveEnabled = projectConfig.GetBool("scene.autosave_enabled", autosaveEnabled);
    autosaveIntervalSeconds = projectConfig.GetInt("scene.autosave_interval_seconds", autosaveIntervalSeconds);

    // editor.toml [assets]
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

    editorScale = NormalizeEditorScale(static_cast<float>(config.GetDouble("ui.editor_scale", editorScale)));

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

    panelOpenStates.clear();
    const int panelCount = std::max(0, config.GetInt("layout.panel_count", 0));
    for (int i = 0; i < panelCount; ++i) {
        const std::string prefix = "layout.panel_" + std::to_string(i);
        const std::string name = config.GetString(prefix + ".name", "");
        if (!name.empty()) {
            SetPanelOpen(name, config.GetBool(prefix + ".open", true));
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

    config.SetDouble("ui.editor_scale", NormalizeEditorScale(editorScale));

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

    config.SetBool("assets.auto_import", assetsAutoImport);
    config.SetBool("assets.auto_cook_dirty", assetsAutoCookDirty);
    config.SetBool("assets.hot_reload", assetsHotReload);

    const int recentCount = static_cast<int>(std::min(recentScenes.size(), kMaxRecentScenes));
    config.SetInt("scene.recent_count", recentCount);
    for (int i = 0; i < recentCount; ++i) {
        const std::string key = "scene.recent_" + std::to_string(i);
        config.SetString(key, recentScenes[static_cast<std::size_t>(i)].generic_string());
    }

    const int panelCount = static_cast<int>(panelOpenStates.size());
    config.SetInt("layout.panel_count", panelCount);
    for (int i = 0; i < panelCount; ++i) {
        const EditorPanelOpenState& panel = panelOpenStates[static_cast<std::size_t>(i)];
        const std::string prefix = "layout.panel_" + std::to_string(i);
        config.SetString(prefix + ".name", panel.name);
        config.SetBool(prefix + ".open", panel.open);
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

bool EditorSettings::PanelOpenOrDefault(std::string_view name, bool defaultOpen) const {
    for (const EditorPanelOpenState& panel : panelOpenStates) {
        if (panel.name == name) {
            return panel.open;
        }
    }
    return defaultOpen;
}

bool EditorSettings::SetPanelOpen(std::string name, bool open) {
    if (name.empty()) {
        return false;
    }
    for (EditorPanelOpenState& panel : panelOpenStates) {
        if (panel.name == name) {
            if (panel.open == open) {
                return false;
            }
            panel.open = open;
            return true;
        }
    }
    panelOpenStates.push_back(EditorPanelOpenState{std::move(name), open});
    return true;
}

bool EditorSettings::SetPanelOpenStates(std::vector<EditorPanelOpenState> states) {
    states.erase(std::remove_if(states.begin(), states.end(),
                                [](const EditorPanelOpenState& panel) { return panel.name.empty(); }),
                 states.end());
    if (panelOpenStates == states) {
        return false;
    }
    panelOpenStates = std::move(states);
    return true;
}

void EditorSettings::ClearPanelOpenStates() {
    panelOpenStates.clear();
}

} // namespace Hockey
