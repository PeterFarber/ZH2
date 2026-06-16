#include "Hockey/Gameplay/Match/MatchClock.hpp"

#include <algorithm>

#include <entt/entt.hpp>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {

void MatchClock::FixedUpdate(Scene& scene, float fixedDeltaSeconds, GameplayEventQueue& events) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    Entity entity(*it, &scene);
    MatchStateComponent& match = entity.GetComponent<MatchStateComponent>();
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
