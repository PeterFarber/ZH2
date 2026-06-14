#include "Hockey/Physics/PhysicsDebugDraw.hpp"

#include <array>
#include <cmath>

namespace Hockey {

namespace {

constexpr float kTwoPi = 6.28318530718f;

glm::vec3 Transform(const glm::vec3& center, const glm::quat& rotation, const glm::vec3& local) {
    return center + rotation * local;
}

// Appends a closed ring of `segments` points lying in the plane spanned by axisU
// and axisV (both already scaled by radius), centred at `center` (world space).
void AppendRing(PhysicsDebugDrawList& list, const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV,
                int segments, const glm::vec4& color) {
    if (segments < 3) {
        segments = 3;
    }
    glm::vec3 prev = center + axisU;
    for (int i = 1; i <= segments; ++i) {
        const float t = (static_cast<float>(i) / static_cast<float>(segments)) * kTwoPi;
        const glm::vec3 point = center + axisU * std::cos(t) + axisV * std::sin(t);
        list.AddLine(prev, point, color);
        prev = point;
    }
}

} // namespace

void AppendWireBox(PhysicsDebugDrawList& list, const glm::vec3& center, const glm::vec3& halfExtents,
                   const glm::quat& rotation, const glm::vec4& color) {
    const std::array<glm::vec3, 8> corners = {
        Transform(center, rotation, {-halfExtents.x, -halfExtents.y, -halfExtents.z}),
        Transform(center, rotation, {halfExtents.x, -halfExtents.y, -halfExtents.z}),
        Transform(center, rotation, {halfExtents.x, halfExtents.y, -halfExtents.z}),
        Transform(center, rotation, {-halfExtents.x, halfExtents.y, -halfExtents.z}),
        Transform(center, rotation, {-halfExtents.x, -halfExtents.y, halfExtents.z}),
        Transform(center, rotation, {halfExtents.x, -halfExtents.y, halfExtents.z}),
        Transform(center, rotation, {halfExtents.x, halfExtents.y, halfExtents.z}),
        Transform(center, rotation, {-halfExtents.x, halfExtents.y, halfExtents.z}),
    };

    static constexpr std::array<std::pair<int, int>, 12> edges = {{
        {0, 1},
        {1, 2},
        {2, 3},
        {3, 0}, // bottom
        {4, 5},
        {5, 6},
        {6, 7},
        {7, 4}, // top
        {0, 4},
        {1, 5},
        {2, 6},
        {3, 7}, // verticals
    }};
    for (const auto& [a, b] : edges) {
        list.AddLine(corners[a], corners[b], color);
    }
}

void AppendWireSphere(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, const glm::vec4& color,
                      int segments) {
    const glm::vec3 x(radius, 0.0f, 0.0f);
    const glm::vec3 y(0.0f, radius, 0.0f);
    const glm::vec3 z(0.0f, 0.0f, radius);
    AppendRing(list, center, x, y, segments, color);
    AppendRing(list, center, x, z, segments, color);
    AppendRing(list, center, y, z, segments, color);
}

void AppendWireCapsule(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, float halfHeight,
                       const glm::quat& rotation, const glm::vec4& color, int segments) {
    // Capsule axis is local Y (matches Jolt's capsule convention).
    const glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = rotation * glm::vec3(radius, 0.0f, 0.0f);
    const glm::vec3 fwd = rotation * glm::vec3(0.0f, 0.0f, radius);

    const glm::vec3 top = center + up * halfHeight;
    const glm::vec3 bottom = center - up * halfHeight;

    AppendRing(list, top, right, fwd, segments, color);
    AppendRing(list, bottom, right, fwd, segments, color);

    // Side lines.
    list.AddLine(top + right, bottom + right, color);
    list.AddLine(top - right, bottom - right, color);
    list.AddLine(top + fwd, bottom + fwd, color);
    list.AddLine(top - fwd, bottom - fwd, color);

    // Cap arcs (half-rings) in the two vertical planes.
    const int half = segments / 2 > 1 ? segments / 2 : 2;
    auto appendArc = [&](const glm::vec3& capCenter, const glm::vec3& planar, const glm::vec3& axis, float dir) {
        glm::vec3 prev = capCenter + planar;
        for (int i = 1; i <= half; ++i) {
            const float t = (static_cast<float>(i) / static_cast<float>(half)) * (kTwoPi * 0.5f);
            const glm::vec3 point = capCenter + planar * std::cos(t) + axis * (std::sin(t) * dir);
            list.AddLine(prev, point, color);
            prev = point;
        }
    };
    appendArc(top, right, up, 1.0f);
    appendArc(top, fwd, up, 1.0f);
    appendArc(bottom, right, up, -1.0f);
    appendArc(bottom, fwd, up, -1.0f);
}

void AppendWireCylinder(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, float halfHeight,
                        const glm::quat& rotation, const glm::vec4& color, int segments) {
    // Cylinder axis is local Y (matches Jolt's cylinder convention).
    const glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = rotation * glm::vec3(radius, 0.0f, 0.0f);
    const glm::vec3 fwd = rotation * glm::vec3(0.0f, 0.0f, radius);

    const glm::vec3 top = center + up * halfHeight;
    const glm::vec3 bottom = center - up * halfHeight;

    AppendRing(list, top, right, fwd, segments, color);
    AppendRing(list, bottom, right, fwd, segments, color);

    list.AddLine(top + right, bottom + right, color);
    list.AddLine(top - right, bottom - right, color);
    list.AddLine(top + fwd, bottom + fwd, color);
    list.AddLine(top - fwd, bottom - fwd, color);
}

void AppendCross(PhysicsDebugDrawList& list, const glm::vec3& center, float size, const glm::vec4& color) {
    list.AddLine(center - glm::vec3(size, 0.0f, 0.0f), center + glm::vec3(size, 0.0f, 0.0f), color);
    list.AddLine(center - glm::vec3(0.0f, size, 0.0f), center + glm::vec3(0.0f, size, 0.0f), color);
    list.AddLine(center - glm::vec3(0.0f, 0.0f, size), center + glm::vec3(0.0f, 0.0f, size), color);
}

} // namespace Hockey
