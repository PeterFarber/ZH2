#include "Hockey/Gameplay/Match/MatchClock.hpp"

#include <algorithm>
#include <cmath>

#include <entt/entt.hpp>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {

void MatchClock::FixedUpdate(Scene& scene,
                             float fixedDeltaSeconds,
                             const GameplaySettings& settings,
                             GameplayEventQueue& events) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    Entity entity(*it, &scene);
    MatchStateComponent& match = entity.GetComponent<MatchStateComponent>();
    if (match.phase == MatchPhase::Warmup) {
        const float previousRemaining = std::max(0.0f, match.phaseTimer);
        match.phaseTimer = std::max(0.0f, match.phaseTimer - fixedDeltaSeconds);

        const int previousSecond = static_cast<int>(std::ceil(previousRemaining));
        const int currentSecond = static_cast<int>(std::ceil(match.phaseTimer));
        if (previousSecond > currentSecond) {
            events.Push({GameplayEventType::CountdownTick});
            if (static_cast<float>(currentSecond) <= settings.countdownBeepStartSeconds) {
                events.Push({GameplayEventType::CountdownBeep});
            }
        }

        if (match.phaseTimer <= 0.0f) {
            match.phase = MatchPhase::FaceoffSetup;
            match.clockRunning = false;
        }
        return;
    }

    if (match.phase != MatchPhase::Playing || !match.clockRunning) {
        return;
    }

    match.periodTimeRemaining = std::max(0.0f, match.periodTimeRemaining - fixedDeltaSeconds);
    if (match.periodTimeRemaining > 0.0f) {
        return;
    }

    match.clockRunning = false;
    events.Push({GameplayEventType::PeriodEnded});
    if (match.period >= match.periodCount) {
        match.phase = MatchPhase::MatchEnded;
        events.Push({GameplayEventType::MatchEnded});
    } else {
        ++match.period;
        match.phase = MatchPhase::PeriodEnded;
    }
}

}
