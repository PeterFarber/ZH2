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
    HK_CHECK_MSG(Contains(header, "ClientLightingShadows"),
                 "Project Settings has a distinct client lighting/shadows page");
    HK_CHECK_MSG(Contains(header, "ServerSimulation"), "Project Settings keeps server simulation separate");
    HK_CHECK_MSG(Contains(header, "ClientStartupScene"), "Project Settings keeps client startup scene separate");
    HK_CHECK_MSG(Contains(header, "ServerStartupScene"), "Project Settings keeps server startup scene separate");

    HK_CHECK_MSG(Contains(source, "Window / Input"), "navigation exposes window/input pages");
    HK_CHECK_MSG(Contains(source, "Lighting & Shadows"), "navigation exposes lighting/shadows pages");
    HK_CHECK_MSG(Contains(source, "Startup Scene"), "navigation exposes startup scene pages");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Editor\")"),
                 "editor navigation labels are scoped to avoid ImGui ID conflicts");
    HK_CHECK_MSG(Contains(source, "ImGui::PushID(\"Client\")"),
                 "client navigation labels are scoped to avoid ImGui ID conflicts");
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
    HK_CHECK_MSG(Contains(source, "SaveClientConfig()"), "Project Settings still writes client config");
    HK_CHECK_MSG(Contains(source, "SaveServerConfig()"), "Project Settings still writes server config");
    HK_CHECK_MSG(!Contains(source, "Allow body checking"), "Project Settings omits removed body-checking setting");
}
