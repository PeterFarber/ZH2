#include "Hockey/Gameplay/Puck/PuckController.hpp"

#include <algorithm>

#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

bool ShouldMove(PuckState state) {
    return state == PuckState::Loose || state == PuckState::Shot || state == PuckState::Passed;
}

} // namespace

void PuckController::FixedUpdate(Scene& scene, const GameplayTuning& tuning, float fixedDeltaSeconds) {
    auto view = scene.Registry().view<PuckComponent, PuckGameplayComponent, PuckRuntimeComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        PuckGameplayComponent& gameplay = view.get<PuckGameplayComponent>(handle);
        PuckRuntimeComponent& runtime = view.get<PuckRuntimeComponent>(handle);
        TransformComponent& transform = view.get<TransformComponent>(handle);

        if (!gameplay.inPlay || !ShouldMove(gameplay.state)) {
            continue;
        }

        gameplay.timeSinceLastTouch += fixedDeltaSeconds;
        transform.localPosition += runtime.velocity * fixedDeltaSeconds;
        runtime.targetPosition = transform.localPosition;

        const float drag = std::max(0.0f, tuning.puck.loosePuckDrag);
        if (drag > 0.0f) {
            runtime.velocity *= std::max(0.0f, 1.0f - drag * fixedDeltaSeconds);
        }

        if (glm::dot(runtime.velocity, runtime.velocity) < 0.0001f) {
            runtime.velocity = glm::vec3{0.0f};
            if (gameplay.state == PuckState::Shot || gameplay.state == PuckState::Passed) {
                gameplay.state = PuckState::Loose;
            }
        }
    }
}

}
