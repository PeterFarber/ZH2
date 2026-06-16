#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

enum class PuckState {
    Loose,
    Possessed,
    Shot,
    Passed,
    Frozen,
    Resetting
};

const char* PuckStateToString(PuckState state);
bool PuckStateFromString(const char* text, PuckState& outState);

struct PuckGameplayComponent {
    PuckState state = PuckState::Loose;
    UUID possessingPlayer;
    UUID lastTouchedPlayer;
    GameplayTeam lastTouchedTeam = GameplayTeam::None;
    float timeSinceLastTouch = 0.0f;
    bool inPlay = true;
};

struct PuckRuntimeComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 targetPosition{0.0f};
};

struct PossessionComponent {
    UUID possessingPlayer;
    GameplayTeam team = GameplayTeam::None;
};

template <> struct ComponentTraits<PuckGameplayComponent> {
    static constexpr const char* Name = "PuckGameplayComponent";
};
template <> struct ComponentTraits<PuckRuntimeComponent> {
    static constexpr const char* Name = "PuckRuntimeComponent";
};
template <> struct ComponentTraits<PossessionComponent> {
    static constexpr const char* Name = "PossessionComponent";
};

}
