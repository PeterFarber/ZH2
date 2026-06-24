#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

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

void RunGameplayTuningPanelContractTests() {
    HockeyTest::BeginSuite("GameplayTuningPanelContractTests");

    const std::string dockspace = ReadProjectFile("libs/editor/include/Hockey/Editor/Dockspace.hpp");
    const std::string app = ReadProjectFile("libs/editor/src/EditorApp.cpp");
    const std::string source = ReadProjectFile("libs/editor/src/Panels/GameplayTuningPanel.cpp");
    const std::string client = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    const std::string server = ReadProjectFile("apps/dedicated_server/src/DedicatedServerApp.cpp");
    const std::string preview = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorGameplayPreview.hpp");

    HK_CHECK_MSG(Contains(dockspace, "kGameplayTuning"), "dockspace exposes Gameplay Tuning panel name");
    HK_CHECK_MSG(Contains(app, "AddPanel<GameplayTuningPanel>"), "EditorApp registers Gameplay Tuning panel");
    HK_CHECK_MSG(Contains(app, "TuningSerializer::Load(Paths::DataFile(\"gameplay/tuning.default.yaml\"))"),
                 "EditorApp loads gameplay tuning YAML");
    HK_CHECK_MSG(Contains(client, "TuningSerializer::Load(Hockey::Paths::DataFile(\"gameplay/tuning.default.yaml\"))"),
                 "game client loads gameplay tuning YAML");
    HK_CHECK_MSG(Contains(server, "TuningSerializer::Load(Hockey::Paths::DataFile(\"gameplay/tuning.default.yaml\"))"),
                 "dedicated server loads gameplay tuning YAML");
    HK_CHECK_MSG(Contains(preview, "void Configure(const GameplaySettings& settings, const GameplayTuning& tuning)"),
                 "editor gameplay preview accepts tuning");
    HK_CHECK_MSG(Contains(source, "tuning.default.yaml"), "panel edits gameplay tuning YAML");

    const char* requiredFields[] = {
        "Max speed", "Acceleration", "Deceleration", "Turn speed degrees", "Boost impulse",
        "Boost cooldown seconds", "Slide stop damping", "Double stop window seconds",
        "Puck control radius", "Steal radius", "Steal cooldown seconds", "Crease move multiplier",
        "Save reach radius", "Boost charges", "Shield radius", "Shield reflect impulse",
        "Possession offset", "Loose puck drag", "Floor Y", "Out of play Y", "Reach", "Width",
        "Poke check cooldown", "Min power", "Max power", "Charge seconds",
        "Self collision grace seconds", "Accuracy degrees", "Assist radius",
        "Max assist angle degrees", "Cooldown", "Impulse", "Radius", "Faceoff delay seconds",
        "Goal detection radius", "Require puck for goal"
    };

    for (const char* field : requiredFields) {
        HK_CHECK_MSG(Contains(source, field), field);
    }
}
