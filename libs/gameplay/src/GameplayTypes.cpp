#include "Hockey/Gameplay/GameplayTypes.hpp"

#include <cstring>

namespace Hockey {

const char* MatchPhaseToString(MatchPhase phase) {
    switch (phase) {
    case MatchPhase::NotStarted: return "NotStarted";
    case MatchPhase::Warmup: return "Warmup";
    case MatchPhase::FaceoffSetup: return "FaceoffSetup";
    case MatchPhase::Faceoff: return "Faceoff";
    case MatchPhase::Playing: return "Playing";
    case MatchPhase::GoalScored: return "GoalScored";
    case MatchPhase::PeriodEnded: return "PeriodEnded";
    case MatchPhase::MatchEnded: return "MatchEnded";
    case MatchPhase::Paused: return "Paused";
    }
    return "NotStarted";
}

bool MatchPhaseFromString(const char* text, MatchPhase& outPhase) {
    constexpr MatchPhase kPhases[] = {MatchPhase::NotStarted,  MatchPhase::Warmup,     MatchPhase::FaceoffSetup,
                                      MatchPhase::Faceoff,     MatchPhase::Playing,    MatchPhase::GoalScored,
                                      MatchPhase::PeriodEnded, MatchPhase::MatchEnded, MatchPhase::Paused};
    for (const MatchPhase phase : kPhases) {
        if (std::strcmp(text, MatchPhaseToString(phase)) == 0) {
            outPhase = phase;
            return true;
        }
    }
    return false;
}

}
