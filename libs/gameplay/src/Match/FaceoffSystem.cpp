#include "Hockey/Gameplay/Match/FaceoffSystem.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {

void FaceoffSystem::FixedUpdate(Scene& scene, float fixedDeltaSeconds, GameplayEventQueue& events) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return;
    }

    Entity entity(*it, &scene);
    MatchStateComponent& match = entity.GetComponent<MatchStateComponent>();
    if (match.phase == MatchPhase::FaceoffSetup) {
        match.phase = MatchPhase::Faceoff;
        match.phaseTimer = 0.0f;
        match.clockRunning = false;
        events.Push({GameplayEventType::FaceoffStarted});
    } else if (match.phase == MatchPhase::Faceoff) {
        match.phaseTimer += fixedDeltaSeconds;
        if (match.phaseTimer >= 1.0f) {
            match.phase = MatchPhase::Playing;
            match.clockRunning = true;
            match.phaseTimer = 0.0f;
            events.Push({GameplayEventType::FaceoffEnded});
            events.Push({GameplayEventType::PeriodStarted});
        }
    }
}

}
