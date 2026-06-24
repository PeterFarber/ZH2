#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

enum class GameplayEventType {
    MatchInitialized,
    MatchStarted,
    CountdownTick,
    CountdownBeep,
    PeriodStarted,
    PeriodEnded,
    FaceoffStarted,
    FaceoffEnded,
    GoalScored,
    ScoreChanged,
    PuckPossessionChanged,
    StealAttempted,
    PlayerBoosted,
    GoalieShieldStarted,
    GoalieShieldEnded,
    PuckShot,
    PuckOutOfPlay,
    ResetStarted,
    ResetCompleted,
    MatchEnded
};

struct GameplayEvent {
    GameplayEventType type = GameplayEventType::MatchInitialized;
    UUID primaryEntity{0};
    UUID secondaryEntity{0};
    GameplayTeam team = GameplayTeam::None;
    glm::vec3 position{0.0f};
    std::string message;
};

const char* GameplayEventTypeToString(GameplayEventType type);

class GameplayEventQueue {
public:
    void Push(GameplayEvent event);
    std::vector<GameplayEvent> Drain();
    void Clear();
    bool Empty() const;
    std::size_t Size() const;

private:
    std::vector<GameplayEvent> m_Events;
};

}
