#include "Hockey/Physics/PhysicsMaterial.hpp"

#include <algorithm>
#include <cmath>

namespace Hockey {

float CombineFriction(const PhysicsMaterial& a, const PhysicsMaterial& b) {
    const float product = a.friction * b.friction;
    if (a.combineFrictionMultiply || b.combineFrictionMultiply) {
        return product;
    }
    return std::sqrt(std::max(0.0f, product)); // geometric mean (Jolt default)
}

float CombineRestitution(const PhysicsMaterial& a, const PhysicsMaterial& b) {
    if (a.combineRestitutionMax || b.combineRestitutionMax) {
        return std::max(a.restitution, b.restitution); // Jolt default
    }
    return 0.5f * (a.restitution + b.restitution);
}

std::vector<PhysicsMaterial> MakeBuiltInMaterials() {
    std::vector<PhysicsMaterial> materials;

    auto add = [&materials](const char* name, float friction, float restitution, float linearDamping,
                            float angularDamping) {
        PhysicsMaterial m;
        m.name = name;
        m.friction = friction;
        m.restitution = restitution;
        m.linearDamping = linearDamping;
        m.angularDamping = angularDamping;
        // Built-ins keep Jolt's default max-restitution combine, so the contact
        // listener reproduces the engine's historical bounce behaviour. Custom
        // materials can clear this to request an averaged response.
        m.combineRestitutionMax = true;
        materials.push_back(m);
    };

    //   name          friction  restitution  linDamp  angDamp
    add("Default", 0.5f, 0.1f, 0.0f, 0.0f);
    add("Ice", 0.05f, 0.05f, 0.0f, 0.01f);
    add("PuckRubber", 0.25f, 0.5f, 0.02f, 0.05f);
    add("PlayerBody", 0.4f, 0.0f, 0.05f, 0.05f);
    add("GoalieBody", 0.4f, 0.0f, 0.05f, 0.05f);
    add("Boards", 0.3f, 0.4f, 0.0f, 0.0f);
    add("Glass", 0.2f, 0.3f, 0.0f, 0.0f);
    add("GoalNet", 0.6f, 0.05f, 0.5f, 0.5f);
    add("Stick", 0.3f, 0.2f, 0.0f, 0.0f);

    // Triggers produce no physical response.
    add("Trigger", 0.0f, 0.0f, 0.0f, 0.0f);

    return materials;
}

PhysicsMaterialRegistry& PhysicsMaterialRegistry::Get() {
    static PhysicsMaterialRegistry instance;
    return instance;
}

void PhysicsMaterialRegistry::RegisterBuiltIns() {
    m_Materials = MakeBuiltInMaterials();
}

void PhysicsMaterialRegistry::Register(const PhysicsMaterial& material) {
    for (auto& existing : m_Materials) {
        if (existing.name == material.name) {
            existing = material;
            return;
        }
    }
    m_Materials.push_back(material);
}

const PhysicsMaterial* PhysicsMaterialRegistry::Find(const std::string& name) const {
    for (const auto& material : m_Materials) {
        if (material.name == name) {
            return &material;
        }
    }
    return nullptr;
}

bool PhysicsMaterialRegistry::Contains(const std::string& name) const {
    return Find(name) != nullptr;
}

const PhysicsMaterial& PhysicsMaterialRegistry::FindOrDefault(const std::string& name) const {
    if (const PhysicsMaterial* found = Find(name)) {
        return *found;
    }
    if (const PhysicsMaterial* fallback = Find("Default")) {
        return *fallback;
    }
    static const PhysicsMaterial kDefault{};
    return kDefault;
}

const std::vector<PhysicsMaterial>& PhysicsMaterialRegistry::All() const {
    return m_Materials;
}

void PhysicsMaterialRegistry::Clear() {
    m_Materials.clear();
}

} // namespace Hockey
