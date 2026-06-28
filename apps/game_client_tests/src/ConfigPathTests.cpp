#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

void TestConfiguredStartupScenesExist() {
    const std::filesystem::path root = Hockey::Paths::Get().root;

    Hockey::Config editorConfig;
    HK_CHECK(editorConfig.Load(root / "data/config/editor.toml"));
    const std::filesystem::path startupScene = root / editorConfig.GetString("scene.startup_scene", "");
    HK_CHECK_MSG(std::filesystem::exists(startupScene), "editor-authored runtime startup scene exists");
}

void TestRuntimeConfigEmbeddingContracts() {
    const std::string gameClientCMake = ReadProjectFile("apps/game_client/CMakeLists.txt");
    const std::string serverCMake = ReadProjectFile("apps/dedicated_server/CMakeLists.txt");
    const std::string gameClientSource = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    const std::string gameClientHeader = ReadProjectFile("apps/game_client/src/GameClientApp.hpp");
    const std::string serverSource = ReadProjectFile("apps/dedicated_server/src/DedicatedServerApp.cpp");

    HK_CHECK_MSG(Contains(gameClientCMake, "hockey_embed_text_resource"),
                 "game client embeds editor.toml defaults from editor-authored data");
    HK_CHECK_MSG(Contains(gameClientCMake, "data/config/editor.toml"),
                 "game client default config source is editor.toml");
    HK_CHECK_MSG(!Contains(gameClientCMake, "data/config/client.toml"),
                 "game client build does not reference client.toml");
    HK_CHECK_MSG(!Contains(gameClientCMake, "copy_if_different") ||
                     !Contains(gameClientCMake, "data/config/client.toml"),
                 "game client build does not copy client.toml as runtime data");
    HK_CHECK_MSG(Contains(serverCMake, "hockey_embed_text_resource"),
                 "dedicated server embeds editor.toml defaults from editor-authored data");
    HK_CHECK_MSG(Contains(serverCMake, "data/config/editor.toml"),
                 "dedicated server default config source is editor.toml");
    HK_CHECK_MSG(!Contains(serverCMake, "data/config/server.toml"),
                 "dedicated server build does not reference server.toml");
    HK_CHECK_MSG(!Contains(serverCMake, "copy_if_different") ||
                     !Contains(serverCMake, "data/config/server.toml"),
                 "dedicated server build does not copy server.toml as runtime data");
    HK_CHECK_MSG(Contains(gameClientSource, "LoadRuntimeConfig"), "game client loads embedded runtime defaults");
    HK_CHECK_MSG(Contains(serverSource, "LoadRuntimeConfig"), "dedicated server loads embedded runtime defaults");
    HK_CHECK_MSG(Contains(gameClientSource, "HockeyGameClient.toml"),
                 "game client uses executable-sibling user config");
    HK_CHECK_MSG(Contains(serverSource, "HockeyDedicatedServer.toml"),
                 "dedicated server uses executable-sibling operator config");
    HK_CHECK_MSG(Contains(gameClientHeader, "m_UserConfigPath"),
                 "game client stores executable-sibling user config path");
    HK_CHECK_MSG(Contains(gameClientSource, "SaveRuntimeUserConfig"),
                 "game client can save user settings to executable-sibling config");
    HK_CHECK_MSG(!Contains(gameClientSource, "Paths::ConfigFile(\"client.toml\")"),
                 "game client does not save player settings to data/config/client.toml");
}

} // namespace

void RunConfigPathTests() {
    HockeyTest::BeginSuite("ConfigPath");
    TestConfiguredStartupScenesExist();
    TestRuntimeConfigEmbeddingContracts();
}
