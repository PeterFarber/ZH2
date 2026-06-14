#include "Hockey/Renderer/DebugDraw.hpp"

#include <array>
#include <cmath>

namespace Hockey {

void DebugDraw::DrawLine(glm::vec3 a, glm::vec3 b, glm::vec4 color) {
    m_Vertices.push_back({a, color});
    m_Vertices.push_back({b, color});
}

void DebugDraw::DrawBox(const glm::mat4& transform, glm::vec4 color) {
    // Unit cube corners in local space (-0.5..0.5).
    std::array<glm::vec3, 8> corners{};
    int i = 0;
    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                const glm::vec4 p = transform * glm::vec4(0.5f * static_cast<float>(x), 0.5f * static_cast<float>(y),
                                                          0.5f * static_cast<float>(z), 1.0f);
                corners[static_cast<size_t>(i++)] = glm::vec3(p);
            }
        }
    }
    // Indices into the corner ordering above (x outer, y mid, z inner).
    constexpr std::array<std::pair<int, int>, 12> edges{
        {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {4, 5}, {4, 6}, {5, 7}, {6, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
    for (const auto& [a, b] : edges) {
        DrawLine(corners[static_cast<size_t>(a)], corners[static_cast<size_t>(b)], color);
    }
}

void DebugDraw::DrawAABB(glm::vec3 min, glm::vec3 max, glm::vec4 color) {
    const glm::vec3 c = (min + max) * 0.5f;
    const glm::vec3 s = (max - min);
    glm::mat4 t(1.0f);
    t[0][0] = s.x;
    t[1][1] = s.y;
    t[2][2] = s.z;
    t[3] = glm::vec4(c, 1.0f);
    DrawBox(t, color);
}

void DebugDraw::DrawSphere(glm::vec3 center, float radius, glm::vec4 color) {
    constexpr int kSegments = 24;
    const float step = 2.0f * 3.14159265358979323846f / static_cast<float>(kSegments);
    for (int i = 0; i < kSegments; ++i) {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>(i + 1);
        const float c0 = std::cos(a0), s0 = std::sin(a0);
        const float c1 = std::cos(a1), s1 = std::sin(a1);
        // Three orthogonal great circles.
        DrawLine(center + glm::vec3(c0, s0, 0) * radius, center + glm::vec3(c1, s1, 0) * radius, color);
        DrawLine(center + glm::vec3(c0, 0, s0) * radius, center + glm::vec3(c1, 0, s1) * radius, color);
        DrawLine(center + glm::vec3(0, c0, s0) * radius, center + glm::vec3(0, c1, s1) * radius, color);
    }
}

void DebugDraw::DrawCapsule(glm::vec3 center, float height, float radius, glm::vec4 color) {
    const float half = std::max(0.0f, height * 0.5f - radius);
    const glm::vec3 top = center + glm::vec3(0, half, 0);
    const glm::vec3 bottom = center - glm::vec3(0, half, 0);
    DrawSphere(top, radius, color);
    DrawSphere(bottom, radius, color);
    // Connecting lines along the body.
    DrawLine(top + glm::vec3(radius, 0, 0), bottom + glm::vec3(radius, 0, 0), color);
    DrawLine(top - glm::vec3(radius, 0, 0), bottom - glm::vec3(radius, 0, 0), color);
    DrawLine(top + glm::vec3(0, 0, radius), bottom + glm::vec3(0, 0, radius), color);
    DrawLine(top - glm::vec3(0, 0, radius), bottom - glm::vec3(0, 0, radius), color);
}

void DebugDraw::Clear() {
    m_Vertices.clear();
}

} // namespace Hockey
