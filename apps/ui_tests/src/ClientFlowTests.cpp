#include "Test.hpp"

#include <filesystem>
#include <string>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/UI/ClientFlow.hpp"
#include "Hockey/UI/ClientFlowSerializer.hpp"

void RunClientFlowTests() {
    HockeyTest::BeginSuite("ClientFlow");

    const std::filesystem::path flowPath = Hockey::Paths::Get().root / "data/ui/flows/default.clientflow.yaml";
    const Hockey::Result<Hockey::ClientFlow> loaded = Hockey::ClientFlowSerializer::Load(flowPath);
    HK_CHECK_MSG(static_cast<bool>(loaded), loaded.error);
    if (!loaded) {
        return;
    }

    const Hockey::ClientFlow& flow = loaded.value;
    HK_CHECK_EQ(flow.startupScreen, Hockey::UIScreenId::Home);
    HK_CHECK_EQ(flow.ScreenDocument(Hockey::UIScreenId::Home), "data/ui/screens/home.rml");
    HK_CHECK_EQ(flow.ScreenDocument(Hockey::UIScreenId::Loading), "data/ui/screens/loading.rml");
    HK_CHECK_EQ(flow.ScreenDocument(Hockey::UIScreenId::MatchHud), "data/ui/screens/match_hud.rml");
    HK_CHECK_EQ(flow.offline.defaultScene, "data/raw/scenes/Main.scene.yaml");
    HK_CHECK(Hockey::ValidateClientFlow(flow).empty());

    Hockey::ClientFlow invalid = flow;
    invalid.screens.home.clear();
    const std::vector<std::string> errors = Hockey::ValidateClientFlow(invalid);
    HK_CHECK_MSG(!errors.empty(), "ClientFlow validation rejects missing home screen");

    Hockey::ClientFlow custom = flow;
    custom.startupScreen = Hockey::UIScreenId::Settings;
    custom.screens.settings = "data/ui/screens/custom_settings.rml";
    custom.backgrounds.home = "data/raw/scenes/MenuBackground.scene.yaml";
    custom.backgrounds.pauseMenu = "data/raw/scenes/PauseBackground.scene.yaml";
    custom.offline.defaultScene = "data/raw/scenes/OfflineRoundTrip.scene.yaml";
    custom.offline.useCurrentEditorSceneWhenPreviewing = false;

    const std::filesystem::path roundTripPath =
        Hockey::Paths::TempFile("client_flow_round_trip.clientflow.yaml");
    const Hockey::Status saved = Hockey::ClientFlowSerializer::Save(custom, roundTripPath);
    HK_CHECK_MSG(static_cast<bool>(saved), saved.error);

    const Hockey::Result<Hockey::ClientFlow> roundTrip = Hockey::ClientFlowSerializer::Load(roundTripPath);
    HK_CHECK_MSG(static_cast<bool>(roundTrip), roundTrip.error);
    if (roundTrip) {
        HK_CHECK_EQ(roundTrip.value.startupScreen, Hockey::UIScreenId::Settings);
        HK_CHECK_EQ(roundTrip.value.screens.settings, std::string("data/ui/screens/custom_settings.rml"));
        HK_CHECK_EQ(roundTrip.value.backgrounds.home, std::string("data/raw/scenes/MenuBackground.scene.yaml"));
        HK_CHECK_EQ(roundTrip.value.backgrounds.pauseMenu, std::string("data/raw/scenes/PauseBackground.scene.yaml"));
        HK_CHECK_EQ(roundTrip.value.offline.defaultScene, std::string("data/raw/scenes/OfflineRoundTrip.scene.yaml"));
        HK_CHECK(!roundTrip.value.offline.useCurrentEditorSceneWhenPreviewing);
    }
}
