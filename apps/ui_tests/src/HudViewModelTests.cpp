#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/UI/ViewModels.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunHudViewModelTests() {
    HockeyTest::BeginSuite("HudViewModel");

    HK_CHECK_EQ(Hockey::FormatClockText(83.0f), std::string("1:23"));
    HK_CHECK_EQ(Hockey::FormatClockText(-3.0f), std::string("0:00"));
    HK_CHECK_EQ(Hockey::FormatShotCharge(0.754f), std::string("75%"));
    HK_CHECK_EQ(Hockey::FormatShotCharge(2.0f), std::string("100%"));

    const std::string uiContext = ReadProjectFile("libs/ui/include/Hockey/UI/RmlUiContext.hpp");
    HK_CHECK_MSG(Contains(uiContext, "SetElementText"), "RmlUiContext exposes stable element text updates");

    const std::string client = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    HK_CHECK_MSG(Contains(client, "BuildRuntimeHudViewModel"), "Game client maps gameplay snapshot data to HudViewModel");
    HK_CHECK_MSG(Contains(client, "ApplyHudViewModel"), "Game client writes HudViewModel fields to RmlUi elements");
    HK_CHECK_MSG(Contains(client, "home-score") && Contains(client, "away-score") && Contains(client, "match-clock"),
                 "Game client binds score and clock HUD element ids");
}
