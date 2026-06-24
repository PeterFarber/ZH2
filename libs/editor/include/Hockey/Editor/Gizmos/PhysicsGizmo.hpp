#pragma once

#include <cstdint>

namespace Hockey {

class DebugDraw;
class EditorContext;
class Scene;
struct PhysicsViewState;

// Draws physics collider/trigger wireframes and rigid-body centres for every
// entity that has physics components, using the renderer's debug lines. The
// geometry is built directly from the ECS components and each entity's live
// world transform, so it works in Edit mode without a running simulation.
namespace PhysicsGizmo {

struct SubmitStats {
    std::uint32_t colliders = 0;
    std::uint32_t triggers = 0;
    std::uint32_t bodyCenters = 0;
    std::uint32_t contacts = 0;
    std::uint32_t lines = 0;

    bool HasAny() const {
        return colliders > 0 || triggers > 0 || bodyCenters > 0 || contacts > 0;
    }
};

SubmitStats Submit(DebugDraw& debug, Scene& scene, const PhysicsViewState& view);
SubmitStats Submit(DebugDraw& debug, EditorContext& context);

} // namespace PhysicsGizmo

} // namespace Hockey
