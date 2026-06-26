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

void RunUIArchitectureContractTests() {
    HockeyTest::BeginSuite("UIArchitectureContracts");

    const std::string rootCMake = ReadProjectFile("CMakeLists.txt");
    const std::string uiCMake = ReadProjectFile("libs/ui/CMakeLists.txt");
    const std::string rendererHeader = ReadProjectFile("libs/renderer/include/Hockey/Renderer/UIOverlay.hpp");
    const std::string rendererCMake = ReadProjectFile("libs/renderer/CMakeLists.txt");
    const std::string gameClientCMake = ReadProjectFile("apps/game_client/CMakeLists.txt");

    HK_CHECK_MSG(Contains(uiCMake, "find_package(RmlUi CONFIG REQUIRED)"), "hockey_ui discovers RmlUi");
    HK_CHECK_MSG(Contains(uiCMake, "RmlUi::RmlUi"), "hockey_ui links RmlUi privately");
    HK_CHECK_MSG(Contains(uiCMake, "hockey_core") && Contains(uiCMake, "hockey_renderer"),
                 "hockey_ui links the core/renderer boundaries it adapts");
    HK_CHECK_MSG(Contains(rootCMake, "HockeyDedicatedServer must not link hockey_ui"),
                 "root CMake protects server from hockey_ui");
    HK_CHECK_MSG(Contains(rootCMake, "HockeyDedicatedServer must not link RmlUi"),
                 "root CMake protects server from RmlUi");
    HK_CHECK_MSG(!Contains(rendererHeader, "RmlUi") && !Contains(rendererHeader, "Rml::"),
                 "renderer UI overlay public API is RmlUi-free");
    HK_CHECK_MSG(!Contains(rendererCMake, "RmlUi::RmlUi"), "hockey_renderer does not link RmlUi");
    HK_CHECK_MSG(Contains(gameClientCMake, "hockey_ui"), "HockeyGameClient links hockey_ui");
}
