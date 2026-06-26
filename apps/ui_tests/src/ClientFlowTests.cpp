#include "Test.hpp"

#include <filesystem>

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
}
