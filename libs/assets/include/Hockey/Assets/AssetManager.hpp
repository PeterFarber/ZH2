#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetEvent.hpp"
#include "Hockey/Assets/AssetRegistry.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/AssetWatcher.hpp"
#include "Hockey/Assets/Cooker.hpp"
#include "Hockey/Assets/Importer.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct AssetManagerCreateInfo {
    std::filesystem::path projectRoot;  // absolute; used to relativize stored paths
    std::filesystem::path rawRoot;      // absolute data/raw
    std::filesystem::path cookedRoot;   // absolute data/cooked
    std::filesystem::path databasePath; // absolute path to asset_database.yaml
};

// Validation report produced by ValidateReferences().
struct AssetValidationIssue {
    AssetID id;
    std::filesystem::path path;
    std::string message;
    bool error = true; // false == warning
};

// Top-level façade over the asset pipeline: owns the database, the importer and
// cooker registries, the runtime registry of loaded assets, and the hot-reload
// watcher. Depends only on hockey_core.
class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Builds a create-info using the engine's standard data/ layout under root.
    static AssetManagerCreateInfo DefaultCreateInfo(const std::filesystem::path& projectRoot);

    Status Init(const AssetManagerCreateInfo& info);
    void Shutdown();

    AssetDatabase& Database() {
        return m_Database;
    }
    const AssetDatabase& Database() const {
        return m_Database;
    }
    AssetRegistry& Registry() {
        return m_Registry;
    }

    const AssetManagerCreateInfo& Info() const {
        return m_Info;
    }

    // Pipeline operations -----------------------------------------------------
    Status DiscoverRawAssets();
    Status ImportAsset(const std::filesystem::path& rawPath);
    Status ImportAll();
    Status CookAsset(AssetID id);
    Status CookAllDirty();
    Status RecookAll();
    Status SaveDatabase();

    // Removes an asset's metadata sidecar and database record (and any live
    // runtime instance). The raw and cooked files are left untouched, so the
    // asset can be re-discovered/re-imported with a fresh id afterwards.
    Status DeleteMetadata(AssetID id);

    // Removes an asset's raw source, metadata sidecar, cooked output, database
    // record, and any live runtime instance. This is the destructive delete
    // used by editor/project-window raw asset deletion.
    Status DeleteAssetFiles(AssetID id);

    // Returns aggregated validation issues; the Status is failed if any error
    // (not just warning) was found.
    Status ValidateReferences(std::vector<AssetValidationIssue>* issues = nullptr);

    // Maps a raw file extension to an asset type (Unknown == unsupported).
    static AssetType ClassifyExtension(const std::filesystem::path& rawPath);

    // Runtime loading. Specialized for the concrete asset types in the .cpp.
    template <typename T> Result<std::shared_ptr<T>> Load(AssetID id);

    void Unload(AssetID id) {
        m_Registry.Unload(id);
    }
    void UnloadAll() {
        m_Registry.UnloadAll();
    }

    // Hot reload --------------------------------------------------------------
    void StartWatching();
    void StopWatching();
    // Polls the watcher, marks changed assets dirty, emits events. Returns the
    // number of changed raw files detected.
    int PollHotReload(bool autoImport, bool autoCookDirty);

    // Event queue. PollEvents() returns and clears the pending events.
    const std::vector<AssetEvent>& PollEvents();
    void PushEvent(const AssetEvent& event);

    Importer* FindImporterForExtension(const std::string& extension) const;
    // Selects an importer for an asset, preferring an exact asset-type match
    // (needed to disambiguate the shared ".yaml" extension between material,
    // scene and prefab) and falling back to extension support.
    Importer* FindImporter(AssetType type, const std::string& extension) const;
    Cooker* FindCooker(AssetType type) const;

private:
    void RegisterPipelines();
    void RegisterImporter(std::unique_ptr<Importer> importer);
    void RegisterCooker(std::unique_ptr<Cooker> cooker);

    std::filesystem::path AbsoluteRaw(const std::filesystem::path& projectRelative) const;
    std::filesystem::path ToProjectRelative(const std::filesystem::path& absolute) const;
    Status ImportMetadata(AssetMetadata& metadata);
    Status CookMetadata(AssetMetadata& metadata);
    // Returns the metadata for `id`, cooking it first if it is dirty/uncooked
    // and a cooker is registered. Returns nullptr (and sets outStatus) on error.
    AssetMetadata* EnsureCooked(AssetID id, Status& outStatus);

    AssetManagerCreateInfo m_Info;
    bool m_Initialized = false;

    AssetDatabase m_Database;
    AssetRegistry m_Registry;
    AssetWatcher m_Watcher;

    std::vector<std::unique_ptr<Importer>> m_Importers;
    std::map<AssetType, std::unique_ptr<Cooker>> m_Cookers;

    std::vector<AssetEvent> m_Events;
    std::vector<AssetEvent> m_PolledEvents;
};

// Forward declarations for the runtime asset types. The Load<T> template is
// only defined for these concrete types (specializations live in the .cpp).
struct TextureAsset;
struct MeshAsset;
struct MaterialAsset;
struct ModelAsset;
struct ShaderAsset;
struct SceneAsset;
struct PrefabAsset;

template <> Result<std::shared_ptr<TextureAsset>> AssetManager::Load<TextureAsset>(AssetID id);
template <> Result<std::shared_ptr<MeshAsset>> AssetManager::Load<MeshAsset>(AssetID id);
template <> Result<std::shared_ptr<ModelAsset>> AssetManager::Load<ModelAsset>(AssetID id);
template <> Result<std::shared_ptr<MaterialAsset>> AssetManager::Load<MaterialAsset>(AssetID id);
template <> Result<std::shared_ptr<ShaderAsset>> AssetManager::Load<ShaderAsset>(AssetID id);
// Scene/prefab assets are lightweight descriptors (id, name, source path,
// dependency ids). ECS owns the runtime scene; these let gameplay/editor code
// resolve a scene/prefab asset id to its cooked metadata + dependency list.
template <> Result<std::shared_ptr<SceneAsset>> AssetManager::Load<SceneAsset>(AssetID id);
template <> Result<std::shared_ptr<PrefabAsset>> AssetManager::Load<PrefabAsset>(AssetID id);

} // namespace Hockey
