#pragma once

#include <cstdint>

#include "Hockey/Core/Config.hpp"

namespace Hockey {

struct GameplaySettings {
    bool enabled = true;

    uint32_t targetPlayerCount = 8;
    uint32_t skatersPerTeam = 3;
    uint32_t goaliesPerTeam = 1;

    float fixedDeltaSeconds = 1.0f / 60.0f;

    float periodLengthSeconds = 180.0f;
    uint32_t periodCount = 3;
    float pregameCountdownSeconds = 10.0f;
    float countdownBeepStartSeconds = 4.0f;

    bool stopClockAfterGoal = true;
    bool autoFaceoffAfterGoal = true;
    float postGoalDelaySeconds = 3.0f;
    float faceoffDelaySeconds = 1.0f;
    float goalDetectionRadius = 1.0f;
    bool requirePuckForGoal = true;
    uint32_t spawnRandomSeed = 0x5A02024u;

    bool allowManualGoalie = true;
    bool allowOutOfPlay = true;

    bool debugDrawGameplay = false;
    bool logGameplayEvents = false;
};

GameplaySettings LoadGameplaySettings(const Config& config);
void SaveGameplaySettings(Config& config, const GameplaySettings& settings);

}
