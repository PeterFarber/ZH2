#pragma once

#include <glm/glm.hpp>

namespace Hockey {

// Builds a TRS matrix: translate * rotate(euler degrees) * scale.
glm::mat4 ComposeTransform(const glm::vec3& position, const glm::vec3& rotationDegrees, const glm::vec3& scale);

// Extracts translation, Euler-degree rotation and scale from a TRS matrix.
// Rotation recovery goes through a quaternion, so the returned Euler angles may
// differ from the original triple while still recomposing to the same matrix.
void DecomposeTransform(const glm::mat4& matrix, glm::vec3& position, glm::vec3& rotationDegrees, glm::vec3& scale);

} // namespace Hockey
