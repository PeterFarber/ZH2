#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace Hockey {

// Immediate-mode debug geometry. Callers submit lines/shapes during a frame;
// the renderer's debug pass draws them after the main scene, then Clear() is
// called at end of frame. Shapes are approximated with line segments.
class DebugDraw {
public:
    struct LineVertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec4 color);
    void DrawBox(const glm::mat4& transform, glm::vec4 color);
    void DrawAABB(glm::vec3 min, glm::vec3 max, glm::vec4 color);
    void DrawSphere(glm::vec3 center, float radius, glm::vec4 color);
    void DrawCapsule(glm::vec3 center, float height, float radius, glm::vec4 color);

    void Clear();
    const std::vector<LineVertex>& Vertices() const {
        return m_Vertices;
    }
    bool Empty() const {
        return m_Vertices.empty();
    }

private:
    std::vector<LineVertex> m_Vertices;
};

} // namespace Hockey
