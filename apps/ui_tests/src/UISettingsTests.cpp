#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/UI/UISettings.hpp"

void RunUISettingsTests() {
    HockeyTest::BeginSuite("UISettings");

    const Hockey::UISettings defaults = Hockey::MakeDefaultUISettings();
    HK_CHECK(defaults.enabled);
    HK_CHECK_NEAR(defaults.uiScale, 1.0f, 0.0001);
    HK_CHECK(!defaults.debuggerEnabled);
    HK_CHECK(!defaults.reloadDocuments);
    HK_CHECK_EQ(defaults.defaultFont, "data/ui/fonts/Inter-Regular.ttf");
    HK_CHECK_EQ(defaults.themeDocument, "data/ui/themes/hockey.rcss");
    HK_CHECK_EQ(defaults.startFlow, "data/ui/flows/default.clientflow.yaml");

    Hockey::Config config;
    config.SetBool("ui.enabled", false);
    config.SetDouble("ui.ui_scale", 1.5);
    config.SetBool("ui.debugger_enabled", true);
    config.SetBool("ui.reload_documents", true);
    config.SetString("ui.default_font", "data/ui/fonts/Test.ttf");
    config.SetString("ui.theme_document", "data/ui/themes/test.rcss");
    config.SetString("ui.start_flow", "data/ui/flows/test.clientflow.yaml");

    const Hockey::UISettings loaded = Hockey::LoadUISettings(config);
    HK_CHECK(!loaded.enabled);
    HK_CHECK_NEAR(loaded.uiScale, 1.5f, 0.0001);
    HK_CHECK(loaded.debuggerEnabled);
    HK_CHECK(loaded.reloadDocuments);
    HK_CHECK_EQ(loaded.defaultFont, "data/ui/fonts/Test.ttf");
    HK_CHECK_EQ(loaded.themeDocument, "data/ui/themes/test.rcss");
    HK_CHECK_EQ(loaded.startFlow, "data/ui/flows/test.clientflow.yaml");

    Hockey::Config saved;
    Hockey::SaveUISettings(saved, loaded);
    HK_CHECK(!saved.GetBool("ui.enabled", true));
    HK_CHECK_NEAR(saved.GetDouble("ui.ui_scale", 0.0), 1.5, 0.0001);
    HK_CHECK(saved.GetBool("ui.debugger_enabled", false));
    HK_CHECK(saved.GetBool("ui.reload_documents", false));
    HK_CHECK_EQ(saved.GetString("ui.start_flow", ""), "data/ui/flows/test.clientflow.yaml");
}
