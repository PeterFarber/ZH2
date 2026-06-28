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

void RunPackagePanelContractTests() {
    HockeyTest::BeginSuite("PackagePanelContractTests");

    const std::string profile = ReadProjectFile("libs/editor/src/Packaging/PackageProfile.cpp");
    const std::string command = ReadProjectFile("libs/editor/src/Packaging/PackageCommand.cpp");
    const std::string panel = ReadProjectFile("libs/editor/src/Panels/PackagePanel.cpp");
    const std::string app = ReadProjectFile("libs/editor/src/EditorApp.cpp");
    const std::string menu = ReadProjectFile("libs/editor/src/MainMenuBar.cpp");
    const std::string dockspace = ReadProjectFile("libs/editor/include/Hockey/Editor/Dockspace.hpp");
    const std::string cmake = ReadProjectFile("libs/editor/CMakeLists.txt");

    HK_CHECK_MSG(Contains(profile, "editor/package_profiles.toml"),
                 "package profiles are editor-owned data");
    HK_CHECK_MSG(Contains(profile, "out/packages/client"), "client package default output exists");
    HK_CHECK_MSG(Contains(profile, "out/packages/server"), "server package default output exists");
    HK_CHECK_MSG(Contains(command, "package_client.ps1") && Contains(command, "package_server.ps1"),
                 "Windows package scripts are command targets");
    HK_CHECK_MSG(Contains(command, "package_client.sh") && Contains(command, "package_server.sh"),
                 "Linux package scripts are command targets");
    HK_CHECK_MSG(Contains(panel, "Package Client"), "Package panel exposes client packaging action");
    HK_CHECK_MSG(Contains(panel, "Package Server"), "Package panel exposes server packaging action");
    HK_CHECK_MSG(Contains(panel, "Clear Log"), "Package panel exposes log clearing");
    HK_CHECK_MSG(Contains(panel, "MakeClientPackageCommand"), "Package panel builds client package command");
    HK_CHECK_MSG(Contains(panel, "MakeServerPackageCommand"), "Package panel builds server package command");
    HK_CHECK_MSG(Contains(panel, "ResolvePackageOutputDir"), "Package panel validates output directories");
    HK_CHECK_MSG(Contains(app, "AddPanel<PackagePanel>"), "EditorApp registers Package panel");
    HK_CHECK_MSG(Contains(app, "OpenPackageWindow"), "EditorApp can focus Package panel");
    HK_CHECK_MSG(Contains(menu, "Package...") && Contains(menu, "OpenPackageWindow"),
                 "File menu opens Package window");
    HK_CHECK_MSG(Contains(dockspace, "kPackage"), "Package panel has a canonical dockspace name");
    HK_CHECK_MSG(Contains(cmake, "src/Packaging/PackageProfile.cpp"), "package profile source is built");
    HK_CHECK_MSG(Contains(cmake, "src/Panels/PackagePanel.cpp"), "package panel source is built");
}
