#include "Test.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>

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
