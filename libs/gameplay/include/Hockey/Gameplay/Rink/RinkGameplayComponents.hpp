#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

struct GoalGameplayComponent {
    GameplayTeam scoringTeam = GameplayTeam::None;
    GameplayTeam defendingTeam = GameplayTeam::None;
    bool enabled = true;
};

struct OutOfPlayComponent {
    glm::vec3 resetPosition{0.0f};
    float minY = -5.0f;
};

struct FaceoffComponent {
    GameplayTeam causeTeam = GameplayTeam::None;
    uint32_t spawnSequence = 0;
    float timer = 0.0f;
    bool locked = false;
    bool useNormalSpawnsForReset = false;
};

template <> struct ComponentTraits<GoalGameplayComponent> {
    static constexpr const char* Name = "GoalGameplayComponent";
};
template <> struct ComponentTraits<OutOfPlayComponent> {
    static constexpr const char* Name = "OutOfPlayComponent";
};
template <> struct ComponentTraits<FaceoffComponent> {
    static constexpr const char* Name = "FaceoffComponent";
};

}
