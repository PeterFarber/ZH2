#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp" // ComponentTraits primary template
#include "Hockey/Physics/PhysicsLayer.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Physics ECS components. These are pure data (no Jolt types) so they live in
// hockey_physics but can be stored in any ecs Scene. hockey_physics consumes
// them to build Jolt bodies, and registers their serializers/metadata with the
// ECS via RegisterPhysicsComponents() (see below) so the ECS never has to
// depend on the physics library.
// ---------------------------------------------------------------------------

enum class RigidBodyType { Static, Kinematic, Dynamic };

const char* RigidBodyTypeToString(RigidBodyType type);
bool RigidBodyTypeFromString(std::string_view text, RigidBodyType& out);

struct RigidBodyComponent {
    RigidBodyType type = RigidBodyType::Static;

    float mass = 1.0f;
    bool useGravity = true;
    bool allowSleeping = true;

    bool lockTranslationX = false;
    bool lockTranslationY = false;
    bool lockTranslationZ = false;

    bool lockRotationX = false;
    bool lockRotationY = false;
    bool lockRotationZ = false;

    float linearDamping = 0.0f;
    float angularDamping = 0.05f;

    glm::vec3 initialLinearVelocity{0.0f};
    glm::vec3 initialAngularVelocity{0.0f};

    PhysicsLayer layer = PhysicsLayer::Static;
    std::string materialName = "Default";
};

struct BoxColliderComponent {
    glm::vec3 halfExtents{0.5f};
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f}; // Euler degrees
    bool isTrigger = false;
};

struct SphereColliderComponent {
    float radius = 0.5f;
    glm::vec3 offset{0.0f};
    bool isTrigger = false;
};

struct CapsuleColliderComponent {
    float radius = 0.5f;
    float halfHeight = 1.0f;
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f}; // Euler degrees
    bool isTrigger = false;
};

struct CylinderColliderComponent {
    float radius = 0.5f;
    float halfHeight = 0.5f;
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f}; // Euler degrees
    bool isTrigger = false;
};

struct MeshColliderComponent {
    // Asset id of the source mesh, stored as a raw uint64 (the AssetID type
    // lives in hockey_assets, which physics must not depend on). 0 == none.
    std::uint64_t meshAsset = 0;
    bool convex = false;
    bool isTrigger = false;
};

struct TriggerComponent {
    std::string tag;
    bool detectPlayers = true;
    bool detectGoalies = true;
    bool detectPuck = true;
};

// Optional per-entity physics-material override. When present on an entity that
// also has a RigidBodyComponent, this material name takes precedence over
// RigidBodyComponent.materialName when the body is created. Lets several bodies
// share one rigid-body setup while overriding friction/restitution per entity.
struct PhysicsMaterialComponent {
    std::string materialName = "Default";
};

struct CharacterControllerComponent {
    float radius = 0.45f;
    float height = 1.8f;
    float stepHeight = 0.25f;
    float maxSlopeDegrees = 45.0f;
};

// ---------------------------------------------------------------------------
// Canonical component names (must match serialized YAML keys + metadata names).
// ---------------------------------------------------------------------------
template <> struct ComponentTraits<RigidBodyComponent> {
    static constexpr const char* Name = "RigidBodyComponent";
};
template <> struct ComponentTraits<BoxColliderComponent> {
    static constexpr const char* Name = "BoxColliderComponent";
};
template <> struct ComponentTraits<SphereColliderComponent> {
    static constexpr const char* Name = "SphereColliderComponent";
};
template <> struct ComponentTraits<CapsuleColliderComponent> {
    static constexpr const char* Name = "CapsuleColliderComponent";
};
template <> struct ComponentTraits<CylinderColliderComponent> {
    static constexpr const char* Name = "CylinderColliderComponent";
};
template <> struct ComponentTraits<MeshColliderComponent> {
    static constexpr const char* Name = "MeshColliderComponent";
};
template <> struct ComponentTraits<TriggerComponent> {
    static constexpr const char* Name = "TriggerComponent";
};
template <> struct ComponentTraits<PhysicsMaterialComponent> {
    static constexpr const char* Name = "PhysicsMaterialComponent";
};
template <> struct ComponentTraits<CharacterControllerComponent> {
    static constexpr const char* Name = "CharacterControllerComponent";
};

// Returns true when an entity has a collider component of any kind.
template <typename EntityT> bool HasAnyCollider(const EntityT& entity) {
    return entity.template HasComponent<BoxColliderComponent>() ||
           entity.template HasComponent<SphereColliderComponent>() ||
           entity.template HasComponent<CapsuleColliderComponent>() ||
           entity.template HasComponent<CylinderColliderComponent>() ||
           entity.template HasComponent<MeshColliderComponent>();
}

// ---------------------------------------------------------------------------
// Registers physics component metadata (inspector), YAML serialization and
// scene validation with the ECS. Call once after ComponentRegistry's
// RegisterPhase2Components(). Idempotent.
// ---------------------------------------------------------------------------
void RegisterPhysicsComponents();

} // namespace Hockey
