#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Hockey {

// Renderer-independent debug geometry. The physics library only *produces* this
// data; an app/editor bridge feeds it to the renderer's DebugDraw. The server
// never consumes it.
struct PhysicsDebugLine {
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    glm::vec4 color{1.0f};
};

struct PhysicsDebugDrawList {
    std::vector<PhysicsDebugLine> lines;

    void Clear() {
        lines.clear();
    }

    void AddLine(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color) {
        lines.push_back({a, b, color});
    }
};

// ---------------------------------------------------------------------------
// Renderer-independent wireframe builders. They append line segments to a draw
// list and are pure glm (no Jolt), so the editor and the physics debug pass can
// share them. Rotations follow the engine convention (local axis transformed by
// the given quaternion, then translated to center).
// ---------------------------------------------------------------------------
void AppendWireBox(PhysicsDebugDrawList& list, const glm::vec3& center, const glm::vec3& halfExtents,
                   const glm::quat& rotation, const glm::vec4& color);
void AppendWireSphere(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, const glm::vec4& color,
                      int segments = 16);
void AppendWireCapsule(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, float halfHeight,
                       const glm::quat& rotation, const glm::vec4& color, int segments = 16);
void AppendWireCylinder(PhysicsDebugDrawList& list, const glm::vec3& center, float radius, float halfHeight,
                        const glm::quat& rotation, const glm::vec4& color, int segments = 16);
void AppendCross(PhysicsDebugDrawList& list, const glm::vec3& center, float size, const glm::vec4& color);

// Standard colours for the physics debug pass.
namespace PhysicsDebugColors {
inline constexpr glm::vec4 kCollider{0.20f, 0.85f, 0.30f, 1.0f};
inline constexpr glm::vec4 kTrigger{0.95f, 0.75f, 0.15f, 1.0f};
inline constexpr glm::vec4 kBodyCenter{0.30f, 0.55f, 0.95f, 1.0f};
inline constexpr glm::vec4 kContact{0.95f, 0.25f, 0.25f, 1.0f};
} // namespace PhysicsDebugColors

} // namespace Hockey
