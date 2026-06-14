#pragma once

#include <glm/glm.hpp>

namespace Hockey {

class DebugDraw;

// Submits a ground-plane grid (XZ) to the renderer's debug-line buffer so it is
// drawn into the viewport's offscreen target. The axis lines through the origin
// are tinted (X red, Z blue); the rest are a faint gray.
namespace GridGizmo {

void Submit(DebugDraw& debug, float spacing, int halfLineCount = 20, const glm::vec3& center = glm::vec3(0.0f));

} // namespace GridGizmo

} // namespace Hockey
