#pragma once

#include <glm/glm.hpp>

namespace Hockey {

struct LightComponent;

// World-space, resolved light data the renderer packs into the scene UBO.
// Type: 0 = directional, 1 = point, 2 = spot (matches the shader convention).
struct LightRenderData {
    int type = 0;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float cosInner = 1.0f;
    float cosOuter = 0.9f;
    bool castsShadows = true;
};

// Resolves a light's world-space position/direction from its world matrix and
// component settings. Direction is the entity's forward (-Z) axis.
LightRenderData BuildLightRenderData(const glm::mat4& worldMatrix, const LightComponent& light);

} // namespace Hockey
