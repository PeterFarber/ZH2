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

void RunProjectSettingsPanelContractTests() {
    HockeyTest::BeginSuite("ProjectSettingsPanelContractTests");

    const std::string header = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/ProjectSettingsPanel.hpp");
    const std::string source = ReadProjectFile("libs/editor/src/Panels/ProjectSettingsPanel.cpp");

    HK_CHECK_MSG(Contains(header, "EditorApplication"), "Project Settings has a distinct editor application page");
    HK_CHECK_MSG(Contains(header, "EditorWindowInput"), "Project Settings has a distinct editor window/input page");
    HK_CHECK_MSG(Contains(header, "EditorLightingShadows"),
                 "Project Settings has a distinct editor lighting/shadows page");
    HK_CHECK_MSG(!Contains(header, "ClientLightingShadows"),
                 "Project Settings does not expose separate client settings pages");
    HK_CHECK_MSG(Contains(header, "ServerSimulation"), "Project Settings keeps server simulation separate");
    HK_CHECK_MSG(Contains(header, "ServerStartupScene"), "Project Settings keeps server startup scene separate");
    HK_CHECK_MSG(!Contains(header, "m_ClientConfig"), "Project Settings does not keep a separate client config");
    HK_CHECK_MSG(Contains(header, "m_DefaultConfig"),
                 "Project Settings keeps code-owned defaults for resetting project config values");
    HK_CHECK_MSG(Contains(header, "m_DefaultRenderer"),
                 "Project Settings keeps renderer defaults for resetting individual renderer fields");
    HK_CHECK_MSG(Contains(header, "m_DefaultPhysics"),
                 "Project Settings keeps physics defaults for resetting individual physics fields");
    HK_CHECK_MSG(Contains(header, "m_DefaultGameplay"),
                 "Project Settings keeps gameplay defaults for resetting individual gameplay fields");

    HK_CHECK_MSG(Contains(source, "Window / Input"), "navigation exposes window/input pages");
    HK_CHECK_MSG(Contains(source, "Lighting & Shadows"), "navigation exposes lighting/shadows pages");
    HK_CHECK_MSG(Contains(source, "Startup Scene"), "navigation exposes startup scene pages");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Editor\")"),
                 "editor navigation labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(!Contains(source, "ImGui::PushID(\"Client\")"),
                 "Project Settings navigation omits separate client scope");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Server\")"),
                 "server navigation labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Directional Filter & Bias\")"),
                 "directional shadow bias labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Contact Shadows\")"),
                 "contact shadow bias labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Local Light Shadows\")"),
                 "local shadow bias labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(Contains(source, "DrawLightingShadowSettings"), "advanced shadow controls are shared");
    HK_CHECK_MSG(Contains(source, "Max rendered lights"), "lighting budget control exists");
    HK_CHECK_MSG(Contains(source, "Max local shadow tiles"), "local shadow tile budget control exists");
    HK_CHECK_MSG(Contains(source, "Directional atlas"), "directional shadow atlas override control exists");
    HK_CHECK_MSG(Contains(source, "Local atlas"), "local shadow atlas override control exists");
    HK_CHECK_MSG(Contains(source, "Split lambda"), "cascade split tuning control exists");
    HK_CHECK_MSG(Contains(source, "Depth bias constant"), "directional depth bias constant control exists");
    HK_CHECK_MSG(Contains(source, "Depth bias slope"), "directional depth bias slope control exists");
    HK_CHECK_MSG(Contains(source, "Contact Shadows"), "contact shadow tuning controls exist");
    HK_CHECK_MSG(Contains(source, "Local Light Shadows"), "local light shadow tuning controls exist");
    HK_CHECK_MSG(Contains(source, "ClampRendererSettings(settings)"),
                 "Project Settings clamps advanced renderer edits before saving");
    HK_CHECK_MSG(Contains(source, "Waypoint prefab"), "Project Settings exposes the waypoint prefab selector");
    HK_CHECK_MSG(Contains(source, "waypointPrefabPath"),
                 "Project Settings edits GameplaySettings waypoint prefab path");
    HK_CHECK_MSG(Contains(source, "kPrefabDragDropType"), "Project Settings accepts prefab drag/drop payloads");
    HK_CHECK_MSG(Contains(source, "SaveServerBuildDefaults"),
                 "Project Settings writes editor-authored server build defaults");
    HK_CHECK_MSG(Contains(source, "MakeDefaultRuntimeConfig"),
                 "Project Settings loads code-owned defaults before editor-authored overrides");
    HK_CHECK_MSG(Contains(source, "Reset Setting"),
                 "Project Settings exposes per-setting reset controls");
    HK_CHECK_MSG(Contains(source, "Reset Page"), "Project Settings exposes page-level reset controls");
    HK_CHECK_MSG(Contains(source, "Reset All Project Settings"),
                 "Project Settings exposes project-wide reset to defaults");
    HK_CHECK_MSG(Contains(source, "Reset Preferences"), "Project Settings exposes user preference reset");
    HK_CHECK_MSG(!Contains(source, "Client Build Defaults"),
                 "Project Settings does not label separate client defaults");
    HK_CHECK_MSG(Contains(source, "Server Build Defaults"),
                 "Project Settings labels server settings as build defaults");
    HK_CHECK_MSG(!Contains(source, "Paths::ConfigFile(\"client.toml\")"),
                 "Project Settings does not load client.toml");
    HK_CHECK_MSG(!Contains(source, "Paths::ConfigFile(\"server.toml\")"),
                 "Project Settings stores server defaults in editor.toml");
    HK_CHECK_MSG(!Contains(source, "Allow body checking"), "Project Settings omits removed body-checking setting");
}
