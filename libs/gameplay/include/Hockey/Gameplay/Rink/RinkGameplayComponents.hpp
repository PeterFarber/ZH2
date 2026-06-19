#pragma once

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

struct FaceoffGameplayComponent {
    int index = 0;
    bool centerIce = false;
    GameplayTeam preferredZone = GameplayTeam::None;
};

struct FaceoffComponent {
    int spotIndex = 0;
    float timer = 0.0f;
    bool locked = false;
};

template <> struct ComponentTraits<GoalGameplayComponent> {
    static constexpr const char* Name = "GoalGameplayComponent";
};
template <> struct ComponentTraits<OutOfPlayComponent> {
    static constexpr const char* Name = "OutOfPlayComponent";
};
template <> struct ComponentTraits<FaceoffGameplayComponent> {
    static constexpr const char* Name = "FaceoffGameplayComponent";
};
template <> struct ComponentTraits<FaceoffComponent> {
    static constexpr const char* Name = "FaceoffComponent";
};

}
