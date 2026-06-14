#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace Hockey {

// Stable 64-bit identifier for a content asset. 0 means "invalid". IDs are
// generated once on import, stored in the metadata sidecar and asset database,
// and must persist across editor restarts and across file moves/renames (as
// long as the sidecar travels with the raw file).
class AssetID {
public:
    AssetID() = default;
    explicit AssetID(uint64_t value) : m_Value(value) {}

    uint64_t Value() const { return m_Value; }
    bool IsValid() const { return m_Value != 0; }
    std::string ToString() const;

    // Produces a nonzero, effectively-unique identifier.
    static AssetID Generate();

    bool operator==(const AssetID& other) const = default;
    bool operator!=(const AssetID& other) const = default;

private:
    uint64_t m_Value = 0;
};

} // namespace Hockey

namespace std {
template <> struct hash<Hockey::AssetID> {
    size_t operator()(const Hockey::AssetID& id) const noexcept {
        return hash<uint64_t>{}(id.Value());
    }
};
} // namespace std
