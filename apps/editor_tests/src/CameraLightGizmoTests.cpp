#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Gizmos/CameraLightGizmo.hpp"
#include "Hockey/Editor/Selection.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"

using namespace Hockey;

namespace {

bool ContainsPosition(const DebugDraw& debug, const glm::vec3& target, float tolerance = 1e-3f) {
    for (const DebugDraw::LineVertex& vertex : debug.Vertices()) {
        if (glm::length(vertex.position - target) <= tolerance) {
            return true;
        }
    }
    return false;
}

CameraRenderData MakeEditorCamera(float aspect) {
    TransformComponent transform;
    transform.localPosition = {0.0f, 0.0f, 10.0f};

    CameraComponent camera;
    camera.fovDegrees = 60.0f;
    camera.nearClip = 0.1f;
    camera.farClip = 100.0f;

    return BuildCameraRenderData(transform, camera, aspect);
}

} // namespace

void RunCameraLightGizmoTests() {
    HockeyTest::BeginSuite("CameraLightGizmoTests");

    // --- selected camera draws a real perspective frustum -------------------
    {
        Scene scene{"CameraGizmo"};
        Entity entity = scene.CreateEntity("Camera");
        auto& camera = entity.AddComponent<CameraComponent>();
        camera.fovDegrees = 90.0f;
        camera.nearClip = 1.0f;
        camera.farClip = 10.0f;

        Selection selection;
        selection.Select(entity.GetUUID());

        DebugDraw debug;
        CameraLightGizmo::Submit(debug, scene, selection, 2.0f);

        HK_CHECK_MSG(ContainsPosition(debug, {20.0f, 10.0f, -10.0f}), "camera far top-right corner");
        HK_CHECK_MSG(ContainsPosition(debug, {-20.0f, -10.0f, -10.0f}), "camera far bottom-left corner");
    }

    // --- selected point light draws range rings at the configured radius ----
    {
        Scene scene{"PointLightGizmo"};
        Entity entity = scene.CreateEntity("Point Light");
        entity.GetComponent<TransformComponent>().localPosition = {1.0f, 2.0f, 3.0f};
        auto& light = entity.AddComponent<LightComponent>();
        light.type = LightComponent::Type::Point;
        light.range = 5.0f;

        Selection selection;
        selection.Select(entity.GetUUID());

        DebugDraw debug;
        CameraLightGizmo::Submit(debug, scene, selection, 1.0f);

        HK_CHECK_MSG(ContainsPosition(debug, {6.0f, 2.0f, 3.0f}), "point light +X range");
        HK_CHECK_MSG(ContainsPosition(debug, {1.0f, 7.0f, 3.0f}), "point light +Y range");
        HK_CHECK_MSG(ContainsPosition(debug, {1.0f, 2.0f, 8.0f}), "point light +Z range");
    }

    // --- selected spot light draws an outer cone from local -Z --------------
    {
        Scene scene{"SpotLightGizmo"};
        Entity entity = scene.CreateEntity("Spot Light");
        auto& light = entity.AddComponent<LightComponent>();
        light.type = LightComponent::Type::Spot;
        light.range = 10.0f;
        light.innerConeDegrees = 20.0f;
        light.outerConeDegrees = 45.0f;

        Selection selection;
        selection.Select(entity.GetUUID());

        DebugDraw debug;
        CameraLightGizmo::Submit(debug, scene, selection, 1.0f);

        HK_CHECK_MSG(ContainsPosition(debug, {0.0f, 0.0f, -10.0f}), "spot light cap center");
        HK_CHECK_MSG(ContainsPosition(debug, {10.0f, 0.0f, -10.0f}), "spot light outer cap +X");
        HK_CHECK_MSG(ContainsPosition(debug, {0.0f, 10.0f, -10.0f}), "spot light outer cap +Y");
    }

    // --- selected directional light rays follow renderer light direction ----
    {
        Scene scene{"DirectionalLightGizmo"};
        Entity entity = scene.CreateEntity("Directional Light");
        auto& light = entity.AddComponent<LightComponent>();
        light.type = LightComponent::Type::Directional;

        Selection selection;
        selection.Select(entity.GetUUID());

        DebugDraw debug;
        CameraLightGizmo::Submit(debug, scene, selection, 1.0f);

        HK_CHECK_MSG(ContainsPosition(debug, {0.0f, 0.0f, -3.0f}), "directional light ray follows local -Z");
    }

    // --- origin picking hits screen-space camera/light handles --------------
    {
        Scene scene{"CameraLightPicking"};
        Entity entity = scene.CreateEntity("Camera");
        entity.AddComponent<CameraComponent>();

        const CameraRenderData editorCamera = MakeEditorCamera(800.0f / 600.0f);
        const UUID centerPick =
            CameraLightGizmo::PickOrigin(scene, editorCamera, {0.5f, 0.5f}, {800.0f, 600.0f});
        HK_CHECK_MSG(centerPick == entity.GetUUID(), "center click picks camera origin");

        const UUID outsidePick =
            CameraLightGizmo::PickOrigin(scene, editorCamera, {0.1f, 0.1f}, {800.0f, 600.0f});
        HK_CHECK_MSG(!outsidePick.IsValid(), "distant click misses camera origin");
    }
}
