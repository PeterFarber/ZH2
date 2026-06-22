#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Gameplay/GameplayTypes.hpp"
#include "Hockey/Gameplay/Match/MatchState.hpp"
#include "Hockey/Gameplay/Puck/PuckComponents.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class Scene;

struct PlayerGameplaySnapshot {
    UUID entity{0};
    uint32_t playerIndex = 0;
    GameplayTeam team = GameplayTeam::None;
    GameplayRole role = GameplayRole::Skater;
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 facingDirection{0.0f, 0.0f, 1.0f};
    float shotChargeRatio = 0.0f;
    bool hasPuck = false;
    bool shotCharging = false;
};

struct PuckGameplaySnapshot {
    UUID entity{0};
    PuckState state = PuckState::Loose;
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    UUID possessingPlayer{0};
};

struct MatchGameplaySnapshot {
    MatchPhase phase = MatchPhase::NotStarted;
    uint32_t period = 1;
    float periodTimeRemaining = 0.0f;
    uint32_t homeScore = 0;
    uint32_t awayScore = 0;
};

struct GameplaySnapshot {
    uint64_t tick = 0;
    MatchGameplaySnapshot match;
    std::vector<PlayerGameplaySnapshot> players;
    PuckGameplaySnapshot puck;
};

GameplaySnapshot BuildGameplaySnapshot(Scene& scene, uint64_t tick);
GameplaySnapshot BuildGameplaySnapshot(Scene& scene, uint64_t tick, const GameplayTuning& tuning);

}
