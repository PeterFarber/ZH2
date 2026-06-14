#pragma once

namespace Hockey {

class DebugDraw;
class Scene;
struct PhysicsViewState;

// Draws physics collider/trigger wireframes and rigid-body centres for every
// entity that has physics components, using the renderer's debug lines. The
// geometry is built directly from the ECS components and each entity's live
// world transform, so it works in Edit mode without a running simulation.
namespace PhysicsGizmo {

void Submit(DebugDraw& debug, Scene& scene, const PhysicsViewState& view);

} // namespace PhysicsGizmo

} // namespace Hockey
