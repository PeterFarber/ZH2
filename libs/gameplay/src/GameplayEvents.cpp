#include "Hockey/Gameplay/GameplayEvents.hpp"

#include <utility>

namespace Hockey {

const char* GameplayEventTypeToString(GameplayEventType type) {
    switch (type) {
    case GameplayEventType::MatchInitialized: return "MatchInitialized";
    case GameplayEventType::MatchStarted: return "MatchStarted";
    case GameplayEventType::CountdownTick: return "CountdownTick";
    case GameplayEventType::CountdownBeep: return "CountdownBeep";
    case GameplayEventType::PeriodStarted: return "PeriodStarted";
    case GameplayEventType::PeriodEnded: return "PeriodEnded";
    case GameplayEventType::FaceoffStarted: return "FaceoffStarted";
    case GameplayEventType::FaceoffEnded: return "FaceoffEnded";
    case GameplayEventType::GoalScored: return "GoalScored";
    case GameplayEventType::ScoreChanged: return "ScoreChanged";
    case GameplayEventType::PuckPossessionChanged: return "PuckPossessionChanged";
    case GameplayEventType::StealAttempted: return "StealAttempted";
    case GameplayEventType::PlayerBoosted: return "PlayerBoosted";
    case GameplayEventType::GoalieShieldStarted: return "GoalieShieldStarted";
    case GameplayEventType::GoalieShieldEnded: return "GoalieShieldEnded";
    case GameplayEventType::PuckShot: return "PuckShot";
    case GameplayEventType::PuckOutOfPlay: return "PuckOutOfPlay";
    case GameplayEventType::ResetStarted: return "ResetStarted";
    case GameplayEventType::ResetCompleted: return "ResetCompleted";
    case GameplayEventType::MatchEnded: return "MatchEnded";
    }
    return "MatchInitialized";
}

void GameplayEventQueue::Push(GameplayEvent event) {
    m_Events.push_back(std::move(event));
}

std::vector<GameplayEvent> GameplayEventQueue::Drain() {
    std::vector<GameplayEvent> drained;
    drained.swap(m_Events);
    return drained;
}

void GameplayEventQueue::Clear() {
    m_Events.clear();
}

bool GameplayEventQueue::Empty() const {
    return m_Events.empty();
}

std::size_t GameplayEventQueue::Size() const {
    return m_Events.size();
}

}
