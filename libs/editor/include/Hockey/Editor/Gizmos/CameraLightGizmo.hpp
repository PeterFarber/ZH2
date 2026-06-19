#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class DebugDraw;
struct CameraRenderData;
class Scene;
class Selection;

// Draws editor-only camera and light handles/volumes into the Scene View debug
// overlay. Small handles are drawn for all cameras/lights; full frustums and
// light volumes are drawn only for selected entities.
namespace CameraLightGizmo {

void Submit(DebugDraw& debug, Scene& scene, const Selection& selection, float viewportAspect);

// Returns the camera/light entity whose origin is within the screen-space pick
// radius. This is used before normal scene picking so entities without visible
// mesh geometry remain easy to select from the viewport.
UUID PickOrigin(Scene& scene, const CameraRenderData& camera, glm::vec2 viewportUV, glm::vec2 viewportPixels,
                float radiusPixels = 14.0f);

} // namespace CameraLightGizmo

} // namespace Hockey
