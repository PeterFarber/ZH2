#pragma once

#include <cstdint>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/GameplayTypes.hpp"

namespace Hockey {

struct MatchStateComponent {
    MatchPhase phase = MatchPhase::NotStarted;
    uint32_t period = 1;
    uint32_t periodCount = 3;
    float periodTimeRemaining = 180.0f;
    float phaseTimer = 0.0f;
    uint32_t homeScore = 0;
    uint32_t awayScore = 0;
    bool clockRunning = false;
    bool matchInitialized = false;
};

struct ScoreComponent {
    uint32_t homeScore = 0;
    uint32_t awayScore = 0;
};

template <> struct ComponentTraits<MatchStateComponent> {
    static constexpr const char* Name = "MatchStateComponent";
};
template <> struct ComponentTraits<ScoreComponent> {
    static constexpr const char* Name = "ScoreComponent";
};

}
