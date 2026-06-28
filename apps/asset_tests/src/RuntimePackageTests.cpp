#include "Test.hpp"

#include "Hockey/Assets/RuntimePackage.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/ResourceProvider.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace {

bool HasPath(const std::vector<Hockey::ResourcePackageEntry>& entries, const std::string& path) {
    return std::any_of(entries.begin(), entries.end(),
                       [&](const Hockey::ResourcePackageEntry& entry) { return entry.path == path; });
}

void WritePackageFixture(const std::filesystem::path& root) {
    Hockey::FileSystem::WriteTextFile(root / "data" / "config" / "client.toml",
                                      "[scene]\nstartup_scene = \"data/raw/scenes/Main.scene.yaml\"\n"
                                      "[gameplay]\nwaypoint_prefab_path = \"data/raw/prefabs/Waypoint.prefab.yaml\"\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "config" / "server.toml",
                                      "[scene]\nstartup_scene = \"data/raw/scenes/Main.scene.yaml\"\n"
                                      "[gameplay]\nwaypoint_prefab_path = \"data/raw/prefabs/Waypoint.prefab.yaml\"\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "gameplay" / "tuning.default.yaml", "GameplayTuning: {}\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "raw" / "scenes" / "Main.scene.yaml", "Scene: Main\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "raw" / "prefabs" / "Waypoint.prefab.yaml", "Prefab: Waypoint\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "ui" / "screens" / "menu.rml", "<rml />\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "shaders" / "bin" / "mesh.vert.spv", "spv");
    Hockey::FileSystem::WriteTextFile(root / "data" / "cooked" / "asset_database.yaml", "Assets: []\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "cooked" / "assets" / "scenes" / "main.scene.yaml",
                                      "Scene: Cooked\n");
    Hockey::FileSystem::WriteTextFile(root / "data" / "cooked" / "assets" / "meshes" / "rink.mesh.bin", "mesh");
}

} // namespace

void RunRuntimePackageTests() {
    HockeyTest::BeginSuite("RuntimePackageTests");

    const std::filesystem::path workspace = Hockey::Paths::TempFile("runtime_package_tests");
    Hockey::FileSystem::Remove(workspace);
    WritePackageFixture(workspace);

    const Hockey::RuntimePackageBuildInfo clientInfo{
        .target = Hockey::RuntimePackageTarget::Client,
        .projectRoot = workspace,
        .configPath = "data/config/client.toml",
    };
    const Hockey::Result<std::vector<Hockey::ResourcePackageEntry>> clientEntries =
        Hockey::BuildRuntimePackageEntries(clientInfo);
    HK_CHECK(clientEntries);
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/config/client.toml"));
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/ui/screens/menu.rml"));
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/shaders/bin/mesh.vert.spv"));
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/gameplay/tuning.default.yaml"));
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/raw/scenes/Main.scene.yaml"));
    HK_CHECK(clientEntries && HasPath(clientEntries.value, "data/cooked/asset_database.yaml"));

    const Hockey::RuntimePackageBuildInfo serverInfo{
        .target = Hockey::RuntimePackageTarget::Server,
        .projectRoot = workspace,
        .configPath = "data/config/server.toml",
    };
    const Hockey::Result<std::vector<Hockey::ResourcePackageEntry>> serverEntries =
        Hockey::BuildRuntimePackageEntries(serverInfo);
    HK_CHECK(serverEntries);
    HK_CHECK(serverEntries && HasPath(serverEntries.value, "data/config/server.toml"));
    HK_CHECK(serverEntries && HasPath(serverEntries.value, "data/gameplay/tuning.default.yaml"));
    HK_CHECK(serverEntries && HasPath(serverEntries.value, "data/raw/scenes/Main.scene.yaml"));
    HK_CHECK(serverEntries && !HasPath(serverEntries.value, "data/ui/screens/menu.rml"));
    HK_CHECK(serverEntries && !HasPath(serverEntries.value, "data/shaders/bin/mesh.vert.spv"));

    const std::filesystem::path packagePath = workspace / "out" / "client_runtime.hkpack";
    HK_CHECK(Hockey::WriteRuntimePackage(clientInfo, packagePath));
    const Hockey::Result<std::vector<Hockey::ResourcePackageEntry>> roundTrip =
        Hockey::ReadResourcePackageFile(packagePath);
    HK_CHECK(roundTrip);
    HK_CHECK(roundTrip && HasPath(roundTrip.value, "data/ui/screens/menu.rml"));

    Hockey::FileSystem::WriteTextFile(workspace / "data" / "config" / "bad.toml",
                                      "[scene]\nstartup_scene = \"../outside.scene.yaml\"\n");
    const Hockey::RuntimePackageBuildInfo badInfo{
        .target = Hockey::RuntimePackageTarget::Client,
        .projectRoot = workspace,
        .configPath = "data/config/bad.toml",
    };
    HK_CHECK(!Hockey::BuildRuntimePackageEntries(badInfo));

    Hockey::FileSystem::Remove(workspace);
}
