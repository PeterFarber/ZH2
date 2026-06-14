#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// In-memory table of all discovered/imported assets, persisted to
// data/cooked/asset_database.yaml. Owns the authoritative AssetMetadata and the
// id<->rawPath indices, and can rebuild the dependents graph from declared
// dependencies.
class AssetDatabase {
public:
    Status Load(const std::filesystem::path& path);
    Status Save(const std::filesystem::path& path) const;

    bool Contains(AssetID id) const;
    bool ContainsPath(const std::filesystem::path& rawPath) const;

    AssetMetadata* Find(AssetID id);
    const AssetMetadata* Find(AssetID id) const;

    AssetMetadata* FindByRawPath(const std::filesystem::path& rawPath);
    const AssetMetadata* FindByRawPath(const std::filesystem::path& rawPath) const;

    // Returns the existing id for a raw path, or creates a fresh metadata record
    // (with a new id) for it. Newly created records start dirty + uncooked.
    AssetID GetOrCreateIDForPath(const std::filesystem::path& rawPath, AssetType type);

    void AddOrUpdate(const AssetMetadata& metadata);
    void Remove(AssetID id);

    std::vector<AssetMetadata*> All();
    std::vector<const AssetMetadata*> All() const;

    std::vector<AssetMetadata*> FindByType(AssetType type);
    std::vector<AssetMetadata*> DirtyAssets();
    std::vector<AssetMetadata*> MissingAssets();

    void MarkDirty(AssetID id);
    // Marks `id` dirty and recursively marks every transitive dependent dirty,
    // so a changed dependency forces its dependents to be recooked.
    void MarkDirtyWithDependents(AssetID id);
    void MarkMissing(AssetID id, bool missing);

    // Recomputes every record's `dependents` list from all records'
    // `dependencies`. Safe to call repeatedly.
    void RebuildDependencyGraph();

    size_t Count() const {
        return m_Assets.size();
    }
    void Clear();

private:
    std::string NormalizedKey(const std::filesystem::path& rawPath) const;
    void RebuildPathIndex();

    std::unordered_map<AssetID, AssetMetadata> m_Assets;
    std::unordered_map<std::string, AssetID> m_PathIndex;
};

} // namespace Hockey
