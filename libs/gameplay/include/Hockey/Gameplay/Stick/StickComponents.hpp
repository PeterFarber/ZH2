#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"

namespace Hockey {

struct StickComponent {
    UUID ownerPlayer{0};
    float reach = 1.5f;
    float width = 0.25f;
    glm::vec3 localOffset{0.0f, 0.0f, 1.0f};
    bool canControlPuck = true;
    bool canShoot = true;
    bool canPass = true;
    bool canCheck = true;
};

struct ShotComponent {
    float charge = 0.0f;
    bool charging = false;
};

struct PassComponent {
    UUID targetPlayer{0};
    float assistRadius = 2.0f;
};

struct CheckComponent {
    float cooldown = 0.0f;
};

template <> struct ComponentTraits<StickComponent> {
    static constexpr const char* Name = "StickComponent";
};
template <> struct ComponentTraits<ShotComponent> {
    static constexpr const char* Name = "ShotComponent";
};
template <> struct ComponentTraits<PassComponent> {
    static constexpr const char* Name = "PassComponent";
};
template <> struct ComponentTraits<CheckComponent> {
    static constexpr const char* Name = "CheckComponent";
};

}
