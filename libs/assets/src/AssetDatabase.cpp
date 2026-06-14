#include "Hockey/Assets/AssetDatabase.hpp"

#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Serialization/AssetDatabaseSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>

namespace Hockey {

std::string AssetDatabase::NormalizedKey(const std::filesystem::path& rawPath) const {
    return rawPath.lexically_normal().generic_string();
}

void AssetDatabase::Clear() {
    m_Assets.clear();
    m_PathIndex.clear();
}

void AssetDatabase::RebuildPathIndex() {
    m_PathIndex.clear();
    for (const auto& [id, metadata] : m_Assets) {
        if (!metadata.rawPath.empty()) {
            m_PathIndex[NormalizedKey(metadata.rawPath)] = id;
        }
    }
}

Status AssetDatabase::Load(const std::filesystem::path& path) {
    Clear();
    if (!FileSystem::Exists(path)) {
        // A missing database file is treated as an empty database.
        return Status::Ok();
    }
    Result<std::vector<AssetMetadata>> loaded = AssetDatabaseSerializer::Load(path);
    if (!loaded) {
        return Status::Fail(loaded.error);
    }
    for (AssetMetadata& metadata : loaded.value) {
        if (metadata.id.IsValid()) {
            m_Assets[metadata.id] = std::move(metadata);
        }
    }
    RebuildPathIndex();
    RebuildDependencyGraph();
    return Status::Ok();
}

Status AssetDatabase::Save(const std::filesystem::path& path) const {
    if (path.has_parent_path()) {
        const Status dirs = FileSystem::CreateDirectories(path.parent_path());
        if (!dirs) {
            return dirs;
        }
    }
    std::vector<const AssetMetadata*> ordered = All();
    // Stable ordering by id keeps the on-disk database diff-friendly.
    std::sort(ordered.begin(), ordered.end(),
              [](const AssetMetadata* a, const AssetMetadata* b) { return a->id.Value() < b->id.Value(); });
    return AssetDatabaseSerializer::Save(path, ordered);
}

bool AssetDatabase::Contains(AssetID id) const {
    return m_Assets.find(id) != m_Assets.end();
}

bool AssetDatabase::ContainsPath(const std::filesystem::path& rawPath) const {
    return m_PathIndex.find(NormalizedKey(rawPath)) != m_PathIndex.end();
}

AssetMetadata* AssetDatabase::Find(AssetID id) {
    auto it = m_Assets.find(id);
    return it == m_Assets.end() ? nullptr : &it->second;
}

const AssetMetadata* AssetDatabase::Find(AssetID id) const {
    auto it = m_Assets.find(id);
    return it == m_Assets.end() ? nullptr : &it->second;
}

AssetMetadata* AssetDatabase::FindByRawPath(const std::filesystem::path& rawPath) {
    auto it = m_PathIndex.find(NormalizedKey(rawPath));
    return it == m_PathIndex.end() ? nullptr : Find(it->second);
}

const AssetMetadata* AssetDatabase::FindByRawPath(const std::filesystem::path& rawPath) const {
    auto it = m_PathIndex.find(NormalizedKey(rawPath));
    return it == m_PathIndex.end() ? nullptr : Find(it->second);
}

AssetID AssetDatabase::GetOrCreateIDForPath(const std::filesystem::path& rawPath, AssetType type) {
    const std::string key = NormalizedKey(rawPath);
    auto it = m_PathIndex.find(key);
    if (it != m_PathIndex.end()) {
        return it->second;
    }
    AssetMetadata metadata;
    metadata.id = AssetID::Generate();
    metadata.type = type;
    metadata.rawPath = rawPath.lexically_normal().generic_string();
    metadata.name = rawPath.stem().generic_string();
    metadata.dirty = true;
    metadata.cooked = false;
    m_Assets[metadata.id] = metadata;
    m_PathIndex[key] = metadata.id;
    return metadata.id;
}

void AssetDatabase::AddOrUpdate(const AssetMetadata& metadata) {
    if (!metadata.id.IsValid()) {
        return;
    }
    // Update the path index if the raw path changed.
    auto existing = m_Assets.find(metadata.id);
    if (existing != m_Assets.end() && !existing->second.rawPath.empty()) {
        const std::string oldKey = NormalizedKey(existing->second.rawPath);
        if (oldKey != NormalizedKey(metadata.rawPath)) {
            m_PathIndex.erase(oldKey);
        }
    }
    m_Assets[metadata.id] = metadata;
    if (!metadata.rawPath.empty()) {
        m_PathIndex[NormalizedKey(metadata.rawPath)] = metadata.id;
    }
}

void AssetDatabase::Remove(AssetID id) {
    auto it = m_Assets.find(id);
    if (it == m_Assets.end()) {
        return;
    }
    if (!it->second.rawPath.empty()) {
        m_PathIndex.erase(NormalizedKey(it->second.rawPath));
    }
    m_Assets.erase(it);
}

std::vector<AssetMetadata*> AssetDatabase::All() {
    std::vector<AssetMetadata*> result;
    result.reserve(m_Assets.size());
    for (auto& [id, metadata] : m_Assets) {
        result.push_back(&metadata);
    }
    return result;
}

std::vector<const AssetMetadata*> AssetDatabase::All() const {
    std::vector<const AssetMetadata*> result;
    result.reserve(m_Assets.size());
    for (const auto& [id, metadata] : m_Assets) {
        result.push_back(&metadata);
    }
    return result;
}

std::vector<AssetMetadata*> AssetDatabase::FindByType(AssetType type) {
    std::vector<AssetMetadata*> result;
    for (auto& [id, metadata] : m_Assets) {
        if (metadata.type == type) {
            result.push_back(&metadata);
        }
    }
    return result;
}

std::vector<AssetMetadata*> AssetDatabase::DirtyAssets() {
    std::vector<AssetMetadata*> result;
    for (auto& [id, metadata] : m_Assets) {
        if (metadata.dirty) {
            result.push_back(&metadata);
        }
    }
    return result;
}

std::vector<AssetMetadata*> AssetDatabase::MissingAssets() {
    std::vector<AssetMetadata*> result;
    for (auto& [id, metadata] : m_Assets) {
        if (metadata.missing) {
            result.push_back(&metadata);
        }
    }
    return result;
}

void AssetDatabase::MarkDirty(AssetID id) {
    if (AssetMetadata* metadata = Find(id)) {
        metadata->dirty = true;
        metadata->cooked = false;
    }
}

void AssetDatabase::MarkDirtyWithDependents(AssetID id) {
    std::vector<AssetID> stack{id};
    std::vector<AssetID> visited;
    while (!stack.empty()) {
        const AssetID current = stack.back();
        stack.pop_back();
        if (std::find(visited.begin(), visited.end(), current) != visited.end()) {
            continue;
        }
        visited.push_back(current);

        AssetMetadata* metadata = Find(current);
        if (metadata == nullptr) {
            continue;
        }
        metadata->dirty = true;
        metadata->cooked = false;
        for (const AssetID dependent : metadata->dependents) {
            stack.push_back(dependent);
        }
    }
}

void AssetDatabase::MarkMissing(AssetID id, bool missing) {
    if (AssetMetadata* metadata = Find(id)) {
        metadata->missing = missing;
    }
}

void AssetDatabase::RebuildDependencyGraph() {
    for (auto& [id, metadata] : m_Assets) {
        metadata.dependents.clear();
    }
    for (auto& [id, metadata] : m_Assets) {
        for (const AssetID dependency : metadata.dependencies) {
            if (AssetMetadata* target = Find(dependency)) {
                if (std::find(target->dependents.begin(), target->dependents.end(), id) == target->dependents.end()) {
                    target->dependents.push_back(id);
                }
            }
        }
    }
}

} // namespace Hockey
