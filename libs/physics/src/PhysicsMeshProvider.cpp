#include "Hockey/Physics/PhysicsMeshProvider.hpp"

#include <utility>

namespace Hockey {

PhysicsMeshRegistry& PhysicsMeshRegistry::Get() {
    static PhysicsMeshRegistry instance;
    return instance;
}

void PhysicsMeshRegistry::SetProvider(PhysicsMeshProvider provider) {
    m_Provider = std::move(provider);
}

bool PhysicsMeshRegistry::HasProvider() const {
    return static_cast<bool>(m_Provider);
}

bool PhysicsMeshRegistry::Resolve(std::uint64_t meshAsset, PhysicsMeshData& out) const {
    if (!m_Provider) {
        return false;
    }
    return m_Provider(meshAsset, out);
}

void PhysicsMeshRegistry::Clear() {
    m_Provider = nullptr;
}

} // namespace Hockey
