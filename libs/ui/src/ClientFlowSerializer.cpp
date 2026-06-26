#include "Hockey/UI/ClientFlowSerializer.hpp"

#include <fstream>
#include <system_error>

#include <yaml-cpp/yaml.h>

namespace Hockey {

namespace {

std::string GetString(const YAML::Node& node, const char* key, std::string fallback = {}) {
    const YAML::Node value = node[key];
    return value && value.IsScalar() ? value.as<std::string>() : std::move(fallback);
}

bool GetBool(const YAML::Node& node, const char* key, bool fallback) {
    const YAML::Node value = node[key];
    return value && value.IsScalar() ? value.as<bool>() : fallback;
}

void EmitOptional(YAML::Emitter& out, const char* key, const std::string& value) {
    if (!value.empty()) {
        out << YAML::Key << key << YAML::Value << value;
    }
}

} // namespace

Result<ClientFlow> ClientFlowSerializer::Load(const std::filesystem::path& path) {
    try {
        const YAML::Node root = YAML::LoadFile(path.string());
        ClientFlow flow = MakeDefaultClientFlow();

        UIScreenId startup = UIScreenId::Home;
        if (FromString(GetString(root, "startup_screen", ToString(flow.startupScreen)), startup)) {
            flow.startupScreen = startup;
        }

        const YAML::Node screens = root["screens"];
        if (screens) {
            flow.screens.home = GetString(screens, "home", flow.screens.home);
            flow.screens.loading = GetString(screens, "loading", flow.screens.loading);
            flow.screens.lobby = GetString(screens, "lobby", flow.screens.lobby);
            flow.screens.teamSelect = GetString(screens, "team_select", flow.screens.teamSelect);
            flow.screens.matchHud = GetString(screens, "match_hud", flow.screens.matchHud);
            flow.screens.pauseMenu = GetString(screens, "pause_menu", flow.screens.pauseMenu);
            flow.screens.settings = GetString(screens, "settings", flow.screens.settings);
            flow.screens.scoreboard = GetString(screens, "scoreboard", flow.screens.scoreboard);
            flow.screens.endMatch = GetString(screens, "end_match", flow.screens.endMatch);
        }

        const YAML::Node backgrounds = root["backgrounds"];
        if (backgrounds) {
            flow.backgrounds.home = GetString(backgrounds, "home", flow.backgrounds.home);
            flow.backgrounds.loading = GetString(backgrounds, "loading", flow.backgrounds.loading);
            flow.backgrounds.lobby = GetString(backgrounds, "lobby", flow.backgrounds.lobby);
            flow.backgrounds.teamSelect = GetString(backgrounds, "team_select", flow.backgrounds.teamSelect);
            flow.backgrounds.pauseMenu = GetString(backgrounds, "pause_menu", flow.backgrounds.pauseMenu);
            flow.backgrounds.settings = GetString(backgrounds, "settings", flow.backgrounds.settings);
            flow.backgrounds.scoreboard = GetString(backgrounds, "scoreboard", flow.backgrounds.scoreboard);
            flow.backgrounds.endMatch = GetString(backgrounds, "end_match", flow.backgrounds.endMatch);
        }

        const YAML::Node offline = root["offline"];
        if (offline) {
            flow.offline.defaultScene = GetString(offline, "default_scene", flow.offline.defaultScene);
            flow.offline.useCurrentEditorSceneWhenPreviewing =
                GetBool(offline, "use_current_editor_scene_when_previewing",
                        flow.offline.useCurrentEditorSceneWhenPreviewing);
        }

        if (const std::vector<std::string> errors = ValidateClientFlow(flow); !errors.empty()) {
            return Result<ClientFlow>::Fail(errors.front());
        }
        return Result<ClientFlow>::Ok(std::move(flow));
    } catch (const YAML::Exception& error) {
        return Result<ClientFlow>::Fail(error.what());
    }
}

Status ClientFlowSerializer::Save(const ClientFlow& flow, const std::filesystem::path& path) {
    if (const std::vector<std::string> errors = ValidateClientFlow(flow); !errors.empty()) {
        return Status::Fail(errors.front());
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "startup_screen" << YAML::Value << ToString(flow.startupScreen);

    out << YAML::Key << "screens" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "home" << YAML::Value << flow.screens.home;
    out << YAML::Key << "loading" << YAML::Value << flow.screens.loading;
    out << YAML::Key << "lobby" << YAML::Value << flow.screens.lobby;
    out << YAML::Key << "team_select" << YAML::Value << flow.screens.teamSelect;
    out << YAML::Key << "match_hud" << YAML::Value << flow.screens.matchHud;
    out << YAML::Key << "pause_menu" << YAML::Value << flow.screens.pauseMenu;
    out << YAML::Key << "settings" << YAML::Value << flow.screens.settings;
    out << YAML::Key << "scoreboard" << YAML::Value << flow.screens.scoreboard;
    out << YAML::Key << "end_match" << YAML::Value << flow.screens.endMatch;
    out << YAML::EndMap;

    out << YAML::Key << "backgrounds" << YAML::Value << YAML::BeginMap;
    EmitOptional(out, "home", flow.backgrounds.home);
    EmitOptional(out, "loading", flow.backgrounds.loading);
    EmitOptional(out, "lobby", flow.backgrounds.lobby);
    EmitOptional(out, "team_select", flow.backgrounds.teamSelect);
    EmitOptional(out, "pause_menu", flow.backgrounds.pauseMenu);
    EmitOptional(out, "settings", flow.backgrounds.settings);
    EmitOptional(out, "scoreboard", flow.backgrounds.scoreboard);
    EmitOptional(out, "end_match", flow.backgrounds.endMatch);
    out << YAML::EndMap;

    out << YAML::Key << "offline" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "default_scene" << YAML::Value << flow.offline.defaultScene;
    out << YAML::Key << "use_current_editor_scene_when_previewing" << YAML::Value
        << flow.offline.useCurrentEditorSceneWhenPreviewing;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return Status::Fail("failed to create client-flow directory: " + ec.message());
        }
    }

    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        return Status::Fail("failed to open client-flow for write: " + path.string());
    }
    stream << out.c_str() << '\n';
    return stream.good() ? Status::Ok() : Status::Fail("failed while writing client-flow: " + path.string());
}

} // namespace Hockey
