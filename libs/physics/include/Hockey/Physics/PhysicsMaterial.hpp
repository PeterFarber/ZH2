#pragma once

#include <string>
#include <vector>

namespace Hockey {

// ---------------------------------------------------------------------------
// A physics material describes the surface response of a body. Stable defaults
// are provided here; final gameplay tuning happens in later phases.
// ---------------------------------------------------------------------------
struct PhysicsMaterial {
    std::string name = "Default";

    float friction = 0.5f;
    float restitution = 0.1f;

    float linearDamping = 0.0f;
    float angularDamping = 0.0f;

    // How a contact's combined values are derived from the two materials.
    // Jolt itself uses geometric/average combine; these flags let later phases
    // request multiply-friction / max-restitution behaviour where it matters.
    bool combineFrictionMultiply = false;
    bool combineRestitutionMax = false;
};

// Combines the friction of two contacting surfaces. Defaults to Jolt's
// geometric mean (sqrt(f1*f2)); if either material opts into
// combineFrictionMultiply the product (f1*f2, a "lower wins" feel) is used.
float CombineFriction(const PhysicsMaterial& a, const PhysicsMaterial& b);

// Combines the restitution (bounciness) of two contacting surfaces. Defaults to
// Jolt's max(r1,r2) (the bouncier surface wins); if neither material requests
// combineRestitutionMax the average is used for a softer response.
float CombineRestitution(const PhysicsMaterial& a, const PhysicsMaterial& b);

// Built-in materials, in a deterministic order. Always contains a "Default".
std::vector<PhysicsMaterial> MakeBuiltInMaterials();

// ---------------------------------------------------------------------------
// Process-wide registry of named materials. Apps/editor populate it once with
// the built-ins (plus any project-specific overrides) and bodies reference
// materials by name.
// ---------------------------------------------------------------------------
class PhysicsMaterialRegistry {
public:
    static PhysicsMaterialRegistry& Get();

    // (Re)installs the built-in material set, discarding any prior entries.
    void RegisterBuiltIns();

    void Register(const PhysicsMaterial& material);

    const PhysicsMaterial* Find(const std::string& name) const;
    bool Contains(const std::string& name) const;

    // Returns the named material, or the "Default" material when not found.
    const PhysicsMaterial& FindOrDefault(const std::string& name) const;

    const std::vector<PhysicsMaterial>& All() const;
    void Clear();

private:
    std::vector<PhysicsMaterial> m_Materials;
};

} // namespace Hockey
