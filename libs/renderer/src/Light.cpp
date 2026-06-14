#include "Hockey/Renderer/Light.hpp"

#include <cmath>

#include "Hockey/ECS/RenderComponents.hpp"

namespace Hockey {

LightRenderData BuildLightRenderData(const glm::mat4& worldMatrix, const LightComponent& light) {
    LightRenderData data;

    switch (light.type) {
    case LightComponent::Type::Directional:
        data.type = 0;
        break;
    case LightComponent::Type::Point:
        data.type = 1;
        break;
    case LightComponent::Type::Spot:
        data.type = 2;
        break;
    }

    data.position = glm::vec3(worldMatrix[3]);
    // Forward (-Z) axis of the world transform, rotation only.
    const glm::vec3 forward = glm::vec3(worldMatrix[2]); // +Z column
    const float len = glm::length(forward);
    data.direction = len > 1e-5f ? -forward / len : glm::vec3(0.0f, -1.0f, 0.0f);

    data.color = light.color;
    data.intensity = light.intensity;
    data.range = light.range;

    const float inner = glm::radians(light.innerConeDegrees);
    const float outer = glm::radians(light.outerConeDegrees);
    data.cosInner = std::cos(inner);
    data.cosOuter = std::cos(outer);
    data.castsShadows = light.castsShadows;

    return data;
}

} // namespace Hockey
