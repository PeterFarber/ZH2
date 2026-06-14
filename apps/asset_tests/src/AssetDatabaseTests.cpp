#include "Test.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

using namespace Hockey;
namespace fs = std::filesystem;

void RunAssetDatabaseTests() {
    HockeyTest::BeginSuite("AssetDatabaseTests");

    const fs::path dbPath = Paths::TempFile("asset_db_test.yaml");
    FileSystem::Remove(dbPath);

    AssetDatabase db;

    // Load empty (missing file) database.
    HK_CHECK_MSG(static_cast<bool>(db.Load(dbPath)), "load missing db ok");
    HK_CHECK_EQ(static_cast<int>(db.Count()), 0);

    // GetOrCreateIDForPath creates a new record and is stable for the same path.
    const AssetID texId = db.GetOrCreateIDForPath("data/raw/textures/ice.png", AssetType::Texture);
    HK_CHECK_MSG(texId.IsValid(), "created texture id valid");
    const AssetID again = db.GetOrCreateIDForPath("data/raw/textures/ice.png", AssetType::Texture);
    HK_CHECK_MSG(texId == again, "same path returns same id");
    HK_CHECK_EQ(static_cast<int>(db.Count()), 1);

    // Find by id and by raw path.
    HK_CHECK_MSG(db.Contains(texId), "contains id");
    HK_CHECK_MSG(db.ContainsPath("data/raw/textures/ice.png"), "contains path");
    HK_CHECK_MSG(db.Find(texId) != nullptr, "find by id");
    HK_CHECK_MSG(db.FindByRawPath("data/raw/textures/ice.png") != nullptr, "find by path");

    // Add a material that depends on the texture.
    AssetMetadata material;
    material.id = AssetID::Generate();
    material.type = AssetType::Material;
    material.rawPath = "data/raw/materials/ice.material.yaml";
    material.name = "ice";
    material.dependencies = {texId};
    material.dirty = true;
    db.AddOrUpdate(material);
    HK_CHECK_EQ(static_cast<int>(db.Count()), 2);

    // Dependency graph: texture should now have the material as a dependent.
    db.RebuildDependencyGraph();
    const AssetMetadata* tex = db.Find(texId);
    HK_CHECK_MSG(tex != nullptr && tex->dependents.size() == 1, "texture has one dependent");
    HK_CHECK_MSG(tex != nullptr && !tex->dependents.empty() && tex->dependents[0] == material.id,
                 "dependent is the material");

    // Find by type.
    HK_CHECK_EQ(static_cast<int>(db.FindByType(AssetType::Texture).size()), 1);
    HK_CHECK_EQ(static_cast<int>(db.FindByType(AssetType::Material).size()), 1);

    // Dirty + missing lists.
    HK_CHECK_EQ(static_cast<int>(db.DirtyAssets().size()), 2);
    db.MarkMissing(texId, true);
    HK_CHECK_EQ(static_cast<int>(db.MissingAssets().size()), 1);
    db.MarkMissing(texId, false);
    HK_CHECK_EQ(static_cast<int>(db.MissingAssets().size()), 0);

    // Save then reload into a fresh database; ids/paths/deps must persist.
    HK_CHECK_MSG(static_cast<bool>(db.Save(dbPath)), "save db ok");

    AssetDatabase reloaded;
    HK_CHECK_MSG(static_cast<bool>(reloaded.Load(dbPath)), "reload db ok");
    HK_CHECK_EQ(static_cast<int>(reloaded.Count()), 2);
    HK_CHECK_MSG(reloaded.Contains(texId), "reloaded contains texture id");
    HK_CHECK_MSG(reloaded.ContainsPath("data/raw/materials/ice.material.yaml"),
                 "reloaded contains material path");
    const AssetMetadata* reloadedMat = reloaded.Find(material.id);
    HK_CHECK_MSG(reloadedMat != nullptr && reloadedMat->dependencies.size() == 1,
                 "reloaded material keeps dependency");
    const AssetMetadata* reloadedTex = reloaded.Find(texId);
    HK_CHECK_MSG(reloadedTex != nullptr && reloadedTex->dependents.size() == 1,
                 "reloaded dependency graph rebuilt");

    // Mark dirty after load.
    reloaded.Find(texId)->dirty = false;
    reloaded.MarkDirty(texId);
    HK_CHECK_MSG(reloaded.Find(texId)->dirty, "mark dirty works");

    // Remove asset.
    reloaded.Remove(material.id);
    HK_CHECK_EQ(static_cast<int>(reloaded.Count()), 1);
    HK_CHECK_MSG(!reloaded.Contains(material.id), "removed asset gone");
    HK_CHECK_MSG(!reloaded.ContainsPath("data/raw/materials/ice.material.yaml"),
                 "removed path gone");

    FileSystem::Remove(dbPath);
}
