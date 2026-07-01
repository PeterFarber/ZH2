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
    const std::string header = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/GameplayTuningPanel.hpp");
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
    HK_CHECK_MSG(Contains(header, "m_DefaultConfig"),
                 "Gameplay Tuning keeps code-owned config defaults for settings reset");
    HK_CHECK_MSG(Contains(header, "m_DefaultSettings"),
                 "Gameplay Tuning keeps gameplay settings defaults for settings reset");
    HK_CHECK_MSG(Contains(source, "MakeDefaultRuntimeConfig"),
                 "Gameplay Tuning loads code-owned defaults before editor-authored overrides");
    HK_CHECK_MSG(Contains(source, "Reset Tuning To Defaults"),
                 "Gameplay Tuning can reset YAML tuning values to struct defaults");
    HK_CHECK_MSG(Contains(source, "Reset Settings To Defaults"),
                 "Gameplay Tuning can reset preview/server settings to code defaults");
    HK_CHECK_MSG(Contains(source, "Hockey/Editor/ImGui/EditorTooltip.hpp"),
                 "Gameplay Tuning routes setting help through the shared tooltip helper");
    HK_CHECK_MSG(Contains(source, "BuildSettingTooltip"),
                 "Gameplay Tuning builds detailed setting tooltips with generated default metadata");
    HK_CHECK_MSG(Contains(source, "Recommended value:"),
                 "Gameplay Tuning tooltips include the recommended reset value");
    HK_CHECK_MSG(Contains(source, "Allowed range:"),
                 "Gameplay Tuning numeric tooltips include allowed ranges");
    HK_CHECK_MSG(Contains(source, "Examples:"),
                 "Gameplay Tuning tooltips include concrete value examples");
    HK_CHECK_MSG(Contains(source, "Performance impact:"),
                 "Gameplay Tuning tooltips include performance or simulation-cost guidance");
    HK_CHECK_MSG(Contains(source, "BuildGeneratedExamplesText"),
                 "Gameplay Tuning generates fallback examples from the specific setting label and value kind");
    HK_CHECK_MSG(Contains(source, "BuildGeneratedPerformanceImpactText"),
                 "Gameplay Tuning generates fallback performance notes from the specific setting category");
    HK_CHECK_MSG(!Contains(source, "Lower values reduce this setting's effect"),
                 "Gameplay Tuning avoids repeated generic example text");
    HK_CHECK_MSG(!Contains(source, "Most gameplay tuning values are read during fixed gameplay or preview ticks"),
                 "Gameplay Tuning avoids repeated generic performance text");
    HK_CHECK_MSG(!Contains(source, "ImGui::SetTooltip"),
                 "Gameplay Tuning avoids raw ImGui tooltip calls");
    HK_CHECK_MSG(Contains(source, "Higher skater max speed makes rushes and breakaways faster"),
                 "Gameplay Tuning skater speed tooltip explains gameplay impact");
    HK_CHECK_MSG(Contains(source, "0.0083 runs gameplay at roughly 120 Hz"),
                 "Gameplay Tuning fixed timestep tooltip gives simulation examples");
    HK_CHECK_MSG(Contains(source, "Longer shot charge raises commitment and lowers shot spam"),
                 "Gameplay Tuning shot charge tooltip explains gameplay pacing");
    HK_CHECK_MSG(Contains(source, "Debug draw adds editor rendering work"),
                 "Gameplay Tuning debug draw tooltip explains performance impact");
    HK_CHECK_MSG(Contains(source, "Editor Preview"), "Gameplay Tuning labels editor preview settings");
    HK_CHECK_MSG(!Contains(source, "Client Build Defaults"), "Gameplay Tuning omits separate client defaults");
    HK_CHECK_MSG(Contains(source, "Server Build Defaults"), "Gameplay Tuning labels server build defaults");
    HK_CHECK_MSG(!Contains(source, "Save Client Build Defaults"),
                 "Gameplay Tuning does not save client build defaults");
    HK_CHECK_MSG(Contains(source, "Save Server Build Defaults"),
                 "Gameplay Tuning saves server build defaults");
    HK_CHECK_MSG(!Contains(source, "Paths::ConfigFile(\"client.toml\")"),
                 "Gameplay Tuning does not load client.toml");
    HK_CHECK_MSG(!Contains(source, "Paths::ConfigFile(\"server.toml\")"),
                 "Gameplay Tuning stores server defaults in editor.toml");

    const char* requiredFields[] = {
        "Max speed", "Acceleration", "Deceleration", "Turn speed degrees", "Boost impulse",
        "Boost cooldown seconds", "Slide stop damping", "Double stop window seconds",
        "Puck control radius", "Steal radius", "Steal cooldown seconds", "Crease move multiplier",
        "Save reach radius", "Boost charges", "Boost recharge seconds", "Shield radius",
        "Shield duration seconds", "Shield cooldown seconds", "Shield reflect impulse",
        "Possession offset", "Loose puck drag", "Floor Y", "Out of play Y", "Reach", "Width",
        "Min power", "Max power", "Charge seconds", "Self collision grace seconds", "Accuracy degrees",
        "Faceoff delay seconds", "Post goal delay seconds", "Goal detection radius", "Require puck for goal",
        "Waypoint prefab", "waypointPrefabPath", "kPrefabDragDropType"
    };

    for (const char* field : requiredFields) {
        HK_CHECK_MSG(Contains(source, field), field);
    }

    const char* forbiddenFields[] = {
        "Poke check cooldown",
        "Assist radius",
        "Max assist angle degrees",
        "Allow body checking",
        "CollapsingHeader(\"Pass\"",
        "CollapsingHeader(\"Check\""
    };

    for (const char* field : forbiddenFields) {
        HK_CHECK_MSG(!Contains(source, field), field);
    }
}
