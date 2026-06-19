#pragma once

namespace Hockey {

enum class MatchPhase {
    NotStarted,
    Warmup,
    FaceoffSetup,
    Faceoff,
    Playing,
    GoalScored,
    PeriodEnded,
    MatchEnded,
    Paused
};

const char* MatchPhaseToString(MatchPhase phase);
bool MatchPhaseFromString(const char* text, MatchPhase& outPhase);

}
