#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace Hockey {
class Scene;
}

namespace HockeyClient {

struct GameplayFollowCameraSettings {
    bool enabled = true;
    uint32_t playerIndex = 0;
    glm::vec3 offset{0.0f, 7.5f, 10.0f};
    glm::vec3 lookAtOffset{0.0f, 1.2f, 0.0f};
    float positionResponse = 9.0f;
    float lookAtResponse = 12.0f;
};

struct GameplayFollowCameraState {
    bool initialized = false;
    glm::vec3 position{0.0f};
    glm::vec3 lookAt{0.0f};
};

bool UpdateGameplayFollowCamera(Hockey::Scene& scene,
                                float deltaSeconds,
                                const GameplayFollowCameraSettings& settings,
                                GameplayFollowCameraState& state);

} // namespace HockeyClient
