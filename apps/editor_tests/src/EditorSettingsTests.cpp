#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/EditorSettings.hpp"

using namespace Hockey;

void RunEditorSettingsTests() {
    HockeyTest::BeginSuite("EditorSettingsTests");

    // --- default settings valid ---------------------------------------------
    {
        EditorSettings defaults;
        HK_CHECK_MSG(defaults.autosaveEnabled, "autosave enabled by default");
        HK_CHECK_EQ(defaults.autosaveIntervalSeconds, 120);
        HK_CHECK_MSG(defaults.validateBeforeSave, "validate before save default");
        HK_CHECK_MSG(defaults.showGrid, "grid shown by default");
        HK_CHECK_NEAR(defaults.gridSpacing, 1.0f, 1e-6);
        HK_CHECK_MSG(!defaults.snapEnabled, "snap disabled by default");
        HK_CHECK_MSG(defaults.restoreLastScene, "restore-last-scene enabled by default");
        HK_CHECK_MSG(defaults.recentScenes.empty(), "no recent scenes by default");
        HK_CHECK_MSG(!defaults.assetsAutoImport, "asset import disabled by default");
        HK_CHECK_MSG(!defaults.assetsAutoCookDirty, "asset cook disabled by default");
        HK_CHECK_MSG(!defaults.assetsHotReload, "asset hot reload disabled by default");
        HK_CHECK_NEAR(defaults.editorScale, 1.0f, 1e-6);
        HK_CHECK_NEAR(EditorSettings::NormalizeEditorScale(0.25f), EditorSettings::kMinEditorScale, 1e-6);
        HK_CHECK_NEAR(EditorSettings::NormalizeEditorScale(3.5f), EditorSettings::kMaxEditorScale, 1e-6);
        HK_CHECK_NEAR(EditorSettings::NormalizeEditorScale(1.25f), 1.25f, 1e-6);
    }

    // --- save then load round-trips every field -----------------------------
    {
        EditorSettings out;
        out.autosaveEnabled = false;
        out.autosaveIntervalSeconds = 45;
        out.validateBeforeSave = false;
        out.validateAfterLoad = false;
        out.showGrid = false;
        out.gridSpacing = 2.5f;
        out.snapEnabled = true;
        out.moveSnap = 0.25f;
        out.rotateSnapDegrees = 30.0f;
        out.scaleSnap = 0.05f;
        out.cameraMoveSpeed = 22.0f;
        out.cameraFastMultiplier = 6.0f;
        out.cameraMouseSensitivity = 0.3f;
        out.restoreLastScene = false;
        out.editorScale = 1.35f;
        out.AddRecentScene("data/raw/scenes/a.scene.yaml");
        out.AddRecentScene("data/raw/scenes/b.scene.yaml");

        const auto path = Paths::TempFile("editor_settings_test.toml");
        HK_CHECK_MSG(static_cast<bool>(out.Save(path)), "saves settings TOML");

        EditorSettings in;
        HK_CHECK_MSG(static_cast<bool>(in.Load(path)), "loads settings TOML");

        HK_CHECK_EQ(in.autosaveEnabled, false);
        HK_CHECK_EQ(in.autosaveIntervalSeconds, 45);
        HK_CHECK_EQ(in.validateBeforeSave, false);
        HK_CHECK_EQ(in.validateAfterLoad, false);
        HK_CHECK_EQ(in.showGrid, false);
        HK_CHECK_NEAR(in.gridSpacing, 2.5f, 1e-5);

        // --- snap settings persist ---
        HK_CHECK_EQ(in.snapEnabled, true);
        HK_CHECK_NEAR(in.moveSnap, 0.25f, 1e-5);
        HK_CHECK_NEAR(in.rotateSnapDegrees, 30.0f, 1e-5);
        HK_CHECK_NEAR(in.scaleSnap, 0.05f, 1e-5);

        // --- camera settings persist ---
        HK_CHECK_NEAR(in.cameraMoveSpeed, 22.0f, 1e-5);
        HK_CHECK_NEAR(in.cameraFastMultiplier, 6.0f, 1e-5);
        HK_CHECK_NEAR(in.cameraMouseSensitivity, 0.3f, 1e-5);
        HK_CHECK_EQ(in.restoreLastScene, false);
        HK_CHECK_NEAR(in.editorScale, 1.35f, 1e-5);

        // --- recent scenes persist (most-recent first) ---
        HK_CHECK_EQ(in.recentScenes.size(), static_cast<std::size_t>(2));
        if (in.recentScenes.size() == 2) {
            HK_CHECK_EQ(in.recentScenes[0].generic_string(), std::string("data/raw/scenes/b.scene.yaml"));
            HK_CHECK_EQ(in.recentScenes[1].generic_string(), std::string("data/raw/scenes/a.scene.yaml"));
        }
    }

    // --- editor scale loads clamped values -------------------------------------
    {
        Config config;
        config.SetDouble("ui.editor_scale", 9.0);

        const auto path = Paths::TempFile("editor_scale_clamp_test.toml");
        HK_CHECK_MSG(static_cast<bool>(config.Save(path)), "writes oversized editor scale");

        EditorSettings in;
        HK_CHECK_MSG(static_cast<bool>(in.Load(path)), "loads oversized editor scale");
        HK_CHECK_NEAR(in.editorScale, EditorSettings::kMaxEditorScale, 1e-5);
    }

    // --- recent scene de-duplication + cap ----------------------------------
    {
        EditorSettings s;
        s.AddRecentScene("one.scene.yaml");
        s.AddRecentScene("two.scene.yaml");
        s.AddRecentScene("one.scene.yaml"); // re-add moves to front, no dup
        HK_CHECK_EQ(s.recentScenes.size(), static_cast<std::size_t>(2));
        HK_CHECK_EQ(s.recentScenes[0].generic_string(), std::string("one.scene.yaml"));

        for (int i = 0; i < 20; ++i) {
            s.AddRecentScene("scene_" + std::to_string(i) + ".scene.yaml");
        }
        HK_CHECK_MSG(s.recentScenes.size() <= EditorSettings::kMaxRecentScenes, "recent scenes capped");
    }

    // --- editor panel open state persists ----------------------------------
    {
        EditorSettings out;
        out.SetPanelOpen("Hierarchy", true);
        out.SetPanelOpen("Console", false);
        out.SetPanelOpen("Project Settings", false);
        out.SetPanelOpen("Console", true); // update existing record without duplicating it

        const auto path = Paths::TempFile("editor_layout_state_test.toml");
        HK_CHECK_MSG(static_cast<bool>(out.Save(path)), "saves panel layout state");

        EditorSettings in;
        HK_CHECK_MSG(static_cast<bool>(in.Load(path)), "loads panel layout state");

        HK_CHECK_EQ(in.panelOpenStates.size(), static_cast<std::size_t>(3));
        HK_CHECK_EQ(in.PanelOpenOrDefault("Hierarchy", false), true);
        HK_CHECK_EQ(in.PanelOpenOrDefault("Console", false), true);
        HK_CHECK_EQ(in.PanelOpenOrDefault("Project Settings", true), false);
        HK_CHECK_EQ(in.PanelOpenOrDefault("Unknown Panel", true), true);
        HK_CHECK_EQ(in.PanelOpenOrDefault("Unknown Panel", false), false);
    }
}
