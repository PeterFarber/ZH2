#include "Test.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Project/FileTypeRegistry.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

using namespace Hockey;

namespace {

std::filesystem::path MakeTempDir(const char* sub) {
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "hockey_project_browser" / sub;
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void WriteFile(const std::filesystem::path& path, const char* text) {
    std::ofstream out(path);
    out << text;
}

AssetMetadata MakeAsset(uint64_t id,
                        AssetType type,
                        const std::filesystem::path& rawPath,
                        const std::filesystem::path& cookedPath,
                        bool cooked = true,
                        bool missing = false,
                        bool dirty = false) {
    AssetMetadata metadata;
    metadata.id = AssetID{id};
    metadata.type = type;
    metadata.rawPath = rawPath;
    metadata.cookedPath = cookedPath;
    metadata.name = rawPath.stem().string();
    metadata.cooked = cooked;
    metadata.missing = missing;
    metadata.dirty = dirty;
    return metadata;
}

} // namespace

void RunProjectBrowserTests() {
    HockeyTest::BeginSuite("ProjectBrowserTests");

    // --- FileTypeRegistry classification -------------------------------------
    {
        HK_CHECK_MSG(FileTypeRegistry::Classify("main.scene.yaml").type == EditorFileType::Scene, "scene type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("rig.prefab.yaml").type == EditorFileType::Prefab, "prefab type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("editor.toml").type == EditorFileType::Toml, "toml type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("art.png").type == EditorFileType::Image, "image type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("readme.md").type == EditorFileType::Text, "text type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("mystery.xyz").type == EditorFileType::Unknown, "unknown type");

        HK_CHECK_MSG(FileTypeRegistry::Classify("a.scene.yaml").supported, "scene is supported");
        // Images and glTF models are now imported by the asset pipeline.
        HK_CHECK_MSG(FileTypeRegistry::Classify("a.png").supported, "image is importable");
        HK_CHECK_MSG(FileTypeRegistry::Classify("m.material.yaml").type == EditorFileType::Material, "material type");
        HK_CHECK_MSG(FileTypeRegistry::Classify("model.gltf").supported, "gltf is importable");

        HK_CHECK_MSG(FileTypeRegistry::IsScene("LEVEL.SCENE.YAML"), "IsScene is case-insensitive");
        HK_CHECK_MSG(FileTypeRegistry::IsPrefab("Thing.Prefab.Yaml"), "IsPrefab is case-insensitive");
        HK_CHECK_MSG(!FileTypeRegistry::IsScene("notes.yaml"), "plain yaml is not a scene");
    }

    // --- Entries: directories first, then files, alphabetically --------------
    {
        const std::filesystem::path dir = MakeTempDir("entries");
        std::error_code ec;
        std::filesystem::create_directories(dir / "zsub", ec);
        WriteFile(dir / "afile.txt", "a");
        WriteFile(dir / "main.scene.yaml", "Scene: {}\nEntities: []");

        ProjectBrowser browser;
        const std::vector<ProjectEntry> entries = browser.Entries(dir);
        HK_CHECK_EQ(entries.size(), static_cast<std::size_t>(3));
        if (entries.size() == 3) {
            HK_CHECK_MSG(entries[0].isDirectory && entries[0].name == "zsub", "directory listed first");
            HK_CHECK_MSG(!entries[1].isDirectory && entries[1].name == "afile.txt", "files alphabetical");
            HK_CHECK_MSG(entries[2].name == "main.scene.yaml" && entries[2].type.type == EditorFileType::Scene,
                         "scene entry classified");
        }
        std::filesystem::remove_all(dir, ec);
    }

    // --- File operations: create / rename / delete ---------------------------
    {
        const std::filesystem::path dir = MakeTempDir("fileops");
        ProjectBrowser browser;

        HK_CHECK_MSG(static_cast<bool>(browser.CreateFolder(dir, "Models")), "create folder");
        HK_CHECK_MSG(std::filesystem::is_directory(dir / "Models"), "folder exists on disk");

        // Creating a duplicate folder fails.
        HK_CHECK_MSG(!browser.CreateFolder(dir, "Models"), "duplicate folder rejected");

        HK_CHECK_MSG(static_cast<bool>(browser.RenameEntry(dir / "Models", "Meshes")), "rename folder");
        HK_CHECK_MSG(!std::filesystem::exists(dir / "Models") && std::filesystem::is_directory(dir / "Meshes"),
                     "rename moved the folder");

        WriteFile(dir / "scratch.txt", "junk");
        HK_CHECK_MSG(static_cast<bool>(browser.DeleteEntry(dir / "scratch.txt")), "delete file");
        HK_CHECK_MSG(!std::filesystem::exists(dir / "scratch.txt"), "file removed from disk");

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // --- Search filter is a case-insensitive substring match -----------------
    {
        ProjectBrowser browser;
        HK_CHECK_MSG(!browser.SearchActive(), "search inactive by default");
        std::snprintf(browser.SearchBuffer(), static_cast<std::size_t>(browser.SearchBufferSize()), "%s", "Cube");
        HK_CHECK_MSG(browser.SearchActive(), "search active after typing");
        HK_CHECK_MSG(browser.MatchesSearch("blue_cube.png"), "substring match (case-insensitive)");
        HK_CHECK_MSG(!browser.MatchesSearch("sphere.obj"), "non-match rejected");
    }

    // --- Cooked project entries expose only cooked, non-missing assets -------
    {
        const EnginePaths& paths = Paths::Get();
        const std::filesystem::path rawRoot = paths.rawAssets;
        const std::filesystem::path cookedRoot = paths.cookedAssets;

        AssetDatabase database;
        database.AddOrUpdate(MakeAsset(1, AssetType::Model, "data/raw/Models/Ship.gltf",
                                       "data/cooked/Models/Ship.model", true));
        database.AddOrUpdate(MakeAsset(2, AssetType::Texture, "data/raw/Models/Ship_Base.png",
                                       "data/cooked/Models/Ship_Base.texture", false));
        database.AddOrUpdate(MakeAsset(3, AssetType::Material, "data/raw/Materials/Missing.material.yaml",
                                       "data/cooked/Materials/Missing.material", true, true));
        database.AddOrUpdate(MakeAsset(4, AssetType::Scene, "data/raw/Scenes/Arena.scene.yaml",
                                       "data/cooked/Scenes/Arena.scene", true));
        database.AddOrUpdate(MakeAsset(5, AssetType::Prefab, "data/raw/ZFolder/Puck.prefab.yaml",
                                       "data/cooked/ZFolder/Puck.prefab", true));

        ProjectBrowser browser;
        const std::vector<CookedProjectEntry> rootEntries = browser.Entries(rawRoot, &database);
        HK_CHECK_EQ(rootEntries.size(), static_cast<std::size_t>(3));
        if (rootEntries.size() == 3) {
            HK_CHECK_MSG(rootEntries[0].isDirectory && rootEntries[0].displayName == "Models",
                         "folders sort before assets and prune missing-only folders");
            HK_CHECK_MSG(rootEntries[1].isDirectory && rootEntries[1].displayName == "Scenes",
                         "scene folder shown because it contains a cooked scene");
            HK_CHECK_MSG(rootEntries[2].isDirectory && rootEntries[2].displayName == "ZFolder",
                         "prefab folder shown because it contains a cooked prefab");
        }

        const std::vector<CookedProjectEntry> modelEntries = browser.Entries(rawRoot / "Models", &database);
        HK_CHECK_EQ(modelEntries.size(), static_cast<std::size_t>(1));
        if (modelEntries.size() == 1) {
            HK_CHECK_MSG(!modelEntries[0].isDirectory, "cooked model appears as selectable asset");
            HK_CHECK_MSG(modelEntries[0].displayName == "Ship.gltf", "asset display name is raw filename");
            HK_CHECK_MSG(modelEntries[0].assetId == AssetID{1}, "asset id preserved");
            HK_CHECK_MSG(modelEntries[0].rawPath == rawRoot / "Models" / "Ship.gltf", "raw path resolved");
            HK_CHECK_MSG(modelEntries[0].cookedPath == cookedRoot / "Models" / "Ship.model", "cooked path resolved");
            HK_CHECK_MSG(modelEntries[0].type.type == EditorFileType::Model, "asset type mapped for project UI");
        }

        const std::vector<CookedProjectEntry> materialEntries = browser.Entries(rawRoot / "Materials", &database);
        HK_CHECK_MSG(materialEntries.empty(), "missing cooked asset is hidden from selectable entries");

        HK_CHECK_MSG(browser.RootForPath(rawRoot / "Models" / "Ship.gltf") == rawRoot, "root matching finds raw root");
        HK_CHECK_MSG(browser.ParentFolderWithinRoots(rawRoot / "Models" / "Nested") == rawRoot / "Models",
                     "parent navigation stays inside root");
        HK_CHECK_MSG(browser.ParentFolderWithinRoots(rawRoot) == rawRoot, "parent navigation clamps at root");
        HK_CHECK_MSG(browser.IsWithinRoots(rawRoot / "Models"), "raw child is within project roots");
        HK_CHECK_MSG(!browser.IsWithinRoots(paths.data / "outside"), "unrelated data path is outside project roots");
    }

    // --- Cooked sections expose assets only and filter by asset type ---------
    {
        AssetDatabase database;
        database.AddOrUpdate(MakeAsset(10, AssetType::Model, "data/raw/models/player.glb",
                                       "data/cooked/assets/models/10.model.bin", true));
        database.AddOrUpdate(MakeAsset(11, AssetType::Material, "data/raw/materials/ice.material.yaml",
                                       "data/cooked/assets/materials/11.material.yaml", true));
        database.AddOrUpdate(MakeAsset(12, AssetType::Scene, "data/raw/scenes/main.scene.yaml",
                                       "data/cooked/assets/scenes/12.scene.yaml", true));
        database.AddOrUpdate(MakeAsset(13, AssetType::Texture, "data/raw/textures/ice.png",
                                       "data/cooked/assets/textures/13.tex.bin", true));
        database.AddOrUpdate(MakeAsset(14, AssetType::Prefab, "data/raw/prefabs/puck.prefab.yaml",
                                       "data/cooked/assets/prefabs/14.prefab.yaml", false));

        ProjectBrowser browser;
        const std::vector<CookedProjectEntry> all = browser.SectionEntries(&database, AssetType::Unknown);
        HK_CHECK_MSG(all.size() == 4, "all cooked section hides uncooked assets");
        HK_CHECK_MSG(std::none_of(all.begin(), all.end(), [](const CookedProjectEntry& entry) {
                         return entry.isDirectory;
                     }),
                     "cooked sections contain assets only");

        const std::vector<CookedProjectEntry> materials = browser.SectionEntries(&database, AssetType::Material);
        HK_CHECK_MSG(materials.size() == 1 && materials[0].assetId == AssetID{11}, "material section filters by type");

        const std::vector<CookedProjectEntry> prefabs = browser.SectionEntries(&database, AssetType::Prefab);
        HK_CHECK_MSG(prefabs.empty(), "prefab section hides uncooked prefabs");
    }

    // --- Selection clears when the selected path disappears ------------------
    {
        const std::filesystem::path dir = MakeTempDir("selection");
        WriteFile(dir / "pick.txt", "x");
        ProjectBrowser browser;
        browser.Select(dir / "pick.txt");
        HK_CHECK_MSG(browser.HasSelection(), "selection set");
        browser.ClearSelectionIfMissing();
        HK_CHECK_MSG(browser.HasSelection(), "selection kept while file exists");

        std::error_code ec;
        std::filesystem::remove(dir / "pick.txt", ec);
        browser.ClearSelectionIfMissing();
        HK_CHECK_MSG(!browser.HasSelection(), "selection cleared once file is gone");
        std::filesystem::remove_all(dir, ec);
    }
}
