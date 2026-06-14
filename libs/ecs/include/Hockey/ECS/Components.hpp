#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Core components
// ---------------------------------------------------------------------------

// Stable identity. Saved to disk and used for hierarchy/prefab/network mapping.
// The runtime entt handle is volatile; this UUID is the durable identity.
struct IDComponent {
    UUID id;
};

struct NameComponent {
    std::string name = "GameObject";
};

struct ActiveComponent {
    bool active = true;            // user-controlled local flag
    bool activeInHierarchy = true; // active && every ancestor active
};

struct TransformComponent {
    glm::vec3 localPosition{0.0f};
    glm::vec3 localRotation{0.0f}; // Euler degrees, editor friendly
    glm::vec3 localScale{1.0f};

    glm::mat4 LocalMatrix() const;
};

// Only present when an entity has a parent. Absence means the entity is a root.
struct ParentComponent {
    UUID parentId{};
};

// Always present on normal GameObjects. Stores child identities by UUID.
struct ChildrenComponent {
    std::vector<UUID> children;
};

// Added to entities created from a prefab asset.
struct PrefabComponent {
    UUID prefabAssetId;  // identifies the prefab asset/file
    UUID sourceEntityId; // original prefab entity UUID before remap
    std::filesystem::path sourcePath;
};

// ---------------------------------------------------------------------------
// Hockey marker components (scene/editor data only, no gameplay simulation)
// ---------------------------------------------------------------------------

enum class Team { None, Home, Away };

enum class PlayerRole { Skater, Goalie };

struct TeamComponent {
    Team team = Team::None;
};

struct PlayerRoleComponent {
    PlayerRole role = PlayerRole::Skater;
};

struct PuckComponent {
    bool startsInPlay = true;
};

struct GoalComponent {
    Team defendingTeam = Team::None;
};

struct SpawnPointComponent {
    Team team = Team::None;
    PlayerRole role = PlayerRole::Skater;
    int index = 0;
};

struct FaceoffSpotComponent {
    int index = 0;
};

struct RinkComponent {
    std::string rinkName = "Default Rink";
};

struct PlayAreaComponent {
    glm::vec3 halfExtents{30.0f, 5.0f, 60.0f};
};

struct CameraRigMarkerComponent {
    std::string purpose = "GameplayCamera";
};

// ---------------------------------------------------------------------------
// Component type -> canonical name. Matches both the serialized YAML key and
// the registered metadata name so events/serialization/metadata stay aligned.
// ---------------------------------------------------------------------------

template <typename T> struct ComponentTraits;

#define HK_DEFINE_COMPONENT_TRAITS(Type)                                                                               \
    template <> struct ComponentTraits<Type> {                                                                         \
        static constexpr const char* Name = #Type;                                                                     \
    }

HK_DEFINE_COMPONENT_TRAITS(IDComponent);
HK_DEFINE_COMPONENT_TRAITS(NameComponent);
HK_DEFINE_COMPONENT_TRAITS(ActiveComponent);
HK_DEFINE_COMPONENT_TRAITS(TransformComponent);
HK_DEFINE_COMPONENT_TRAITS(ParentComponent);
HK_DEFINE_COMPONENT_TRAITS(ChildrenComponent);
HK_DEFINE_COMPONENT_TRAITS(PrefabComponent);
HK_DEFINE_COMPONENT_TRAITS(TeamComponent);
HK_DEFINE_COMPONENT_TRAITS(PlayerRoleComponent);
HK_DEFINE_COMPONENT_TRAITS(PuckComponent);
HK_DEFINE_COMPONENT_TRAITS(GoalComponent);
HK_DEFINE_COMPONENT_TRAITS(SpawnPointComponent);
HK_DEFINE_COMPONENT_TRAITS(FaceoffSpotComponent);
HK_DEFINE_COMPONENT_TRAITS(RinkComponent);
HK_DEFINE_COMPONENT_TRAITS(PlayAreaComponent);
HK_DEFINE_COMPONENT_TRAITS(CameraRigMarkerComponent);

#undef HK_DEFINE_COMPONENT_TRAITS

template <typename T> constexpr const char* ComponentName() {
    return ComponentTraits<T>::Name;
}

} // namespace Hockey
