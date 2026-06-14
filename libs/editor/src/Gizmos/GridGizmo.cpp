#include "Hockey/Editor/Gizmos/GridGizmo.hpp"

#include <algorithm>
#include <cmath>

#include "Hockey/Renderer/DebugDraw.hpp"

namespace Hockey::GridGizmo {

namespace {
constexpr glm::vec4 kLineColor{0.32f, 0.32f, 0.36f, 1.0f};
constexpr glm::vec4 kAxisX{0.72f, 0.25f, 0.25f, 1.0f};
constexpr glm::vec4 kAxisZ{0.25f, 0.35f, 0.75f, 1.0f};
} // namespace

void Submit(DebugDraw& debug, float spacing, int halfLineCount, const glm::vec3& center) {
    if (spacing <= 0.0f) {
        spacing = 1.0f;
    }
    halfLineCount = std::clamp(halfLineCount, 1, 256);

    // Snap the grid origin to the spacing so it stays aligned as the camera pans.
    const float originX = std::round(center.x / spacing) * spacing;
    const float originZ = std::round(center.z / spacing) * spacing;
    const float extent = static_cast<float>(halfLineCount) * spacing;
    const float y = center.y;

    for (int i = -halfLineCount; i <= halfLineCount; ++i) {
        const float offset = static_cast<float>(i) * spacing;

        // Lines parallel to X (varying Z).
        const float z = originZ + offset;
        const glm::vec4 colorZ = (i == 0) ? kAxisX : kLineColor;
        debug.DrawLine({originX - extent, y, z}, {originX + extent, y, z}, colorZ);

        // Lines parallel to Z (varying X).
        const float x = originX + offset;
        const glm::vec4 colorX = (i == 0) ? kAxisZ : kLineColor;
        debug.DrawLine({x, y, originZ - extent}, {x, y, originZ + extent}, colorX);
    }
}

} // namespace Hockey::GridGizmo
