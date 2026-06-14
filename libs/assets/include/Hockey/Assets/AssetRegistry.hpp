#pragma once
#include <memory>
#include <unordered_map>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Runtime cache mapping AssetID -> loaded CPU-side asset object. Type erasure
// via std::shared_ptr<void> lets a single registry hold any asset type while
// Get<T> recovers the typed pointer. The renderer keeps GPU copies separately.
class AssetRegistry {
public:
    template <typename T> std::shared_ptr<T> Get(AssetID id) {
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<T>(it->second);
    }

    template <typename T> void Store(AssetID id, std::shared_ptr<T> asset) {
        m_Assets[id] = std::static_pointer_cast<void>(std::move(asset));
    }

    void Unload(AssetID id) { m_Assets.erase(id); }
    void UnloadAll() { m_Assets.clear(); }

    bool IsLoaded(AssetID id) const { return m_Assets.find(id) != m_Assets.end(); }

    size_t Count() const { return m_Assets.size(); }

private:
    std::unordered_map<AssetID, std::shared_ptr<void>> m_Assets;
};

} // namespace Hockey
