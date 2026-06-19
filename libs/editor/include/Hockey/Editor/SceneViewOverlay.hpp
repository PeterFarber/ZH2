#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class EditorContext;
class Scene;
struct CameraRenderData;

enum class SceneViewAxis {
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ,
    Isometric,
};

struct SceneViewProjection {
    bool visible = false;
    glm::vec2 pixels{0.0f};
    float depth = 0.0f;
};

struct SceneViewOverlayResult {
    bool hovered = false;
    bool capturedMouse = false;
};

namespace SceneViewOverlay {

SceneViewProjection ProjectWorldToViewportPixels(const CameraRenderData& camera, glm::vec3 worldPosition,
                                                 glm::vec2 viewportPixels);
glm::vec3 ForwardForAxis(SceneViewAxis axis);
UUID PickIcon(Scene& scene, const CameraRenderData& camera, glm::vec2 viewportPixels, glm::vec2 mousePixels,
              float radiusPixels = 18.0f);
bool ContainsControls(glm::vec2 imagePos, glm::vec2 imageSize, glm::vec2 mousePos);

SceneViewOverlayResult Draw(EditorContext& context, Scene& scene, const CameraRenderData& sceneCamera,
                            glm::vec2 imagePos, glm::vec2 imageSize, bool hasActiveGameCamera,
                            const CameraRenderData& gameCamera, bool& followGameCamera);

} // namespace SceneViewOverlay

} // namespace Hockey
