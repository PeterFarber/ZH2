#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCamera.hpp"
#include "Hockey/Editor/SceneViewOverlay.hpp"
#include "Hockey/Renderer/Camera.hpp"

using namespace Hockey;

namespace {

CameraRenderData MakeOverlayCamera(float aspect) {
    TransformComponent transform;
    transform.localPosition = {0.0f, 0.0f, 10.0f};

    CameraComponent camera;
    camera.fovDegrees = 60.0f;
    camera.nearClip = 0.1f;
    camera.farClip = 100.0f;

    return BuildCameraRenderData(transform, camera, aspect);
}

} // namespace

void RunSceneViewOverlayTests() {
    HockeyTest::BeginSuite("SceneViewOverlayTests");

    // --- world projection maps scene-space origins into viewport pixels -----
    {
        const CameraRenderData camera = MakeOverlayCamera(800.0f / 600.0f);
        const SceneViewProjection center =
            SceneViewOverlay::ProjectWorldToViewportPixels(camera, {0.0f, 0.0f, 0.0f}, {800.0f, 600.0f});
        HK_CHECK_MSG(center.visible, "center point projects into the viewport");
        HK_CHECK_NEAR(center.pixels.x, 400.0f, 1e-3);
        HK_CHECK_NEAR(center.pixels.y, 300.0f, 1e-3);

        const SceneViewProjection outside =
            SceneViewOverlay::ProjectWorldToViewportPixels(camera, {1000.0f, 0.0f, 0.0f}, {800.0f, 600.0f});
        HK_CHECK_MSG(!outside.visible, "off-screen point is rejected");
    }

    // --- marker icon classification covers meshless authored markers -----------
    {
        Scene scene{"MarkerIconClassification"};

        Entity spawn = scene.CreateEntity("Spawn");
        spawn.AddComponent<SpawnPointComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(spawn) == SceneViewIconKind::Spawn,
                     "spawn point gets a scene icon");

        Entity faceoff = scene.CreateEntity("Faceoff");
        auto& faceoffSpawn = faceoff.AddComponent<SpawnPointComponent>();
        faceoffSpawn.faceoffSpawn = true;
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(faceoff) == SceneViewIconKind::Faceoff,
                     "faceoff spawn gets a scene icon");

        Entity puck = scene.CreateEntity("Puck");
        puck.AddComponent<PuckComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(puck) == SceneViewIconKind::Puck,
                     "meshless puck marker gets a scene icon");

        Entity goal = scene.CreateEntity("Goal");
        goal.AddComponent<GoalComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(goal) == SceneViewIconKind::Goal,
                     "meshless goal marker gets a scene icon");

        Entity rink = scene.CreateEntity("Rink");
        rink.AddComponent<RinkComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(rink) == SceneViewIconKind::Rink,
                     "meshless rink marker gets a scene icon");

        Entity playArea = scene.CreateEntity("Play Area");
        playArea.AddComponent<PlayAreaComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(playArea) == SceneViewIconKind::PlayArea,
                     "play area marker gets a scene icon");

        Entity rig = scene.CreateEntity("Camera Rig");
        rig.AddComponent<CameraRigMarkerComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(rig) == SceneViewIconKind::CameraRig,
                     "camera rig marker gets a scene icon");
    }

    // --- non-spatial metadata and visible meshes do not get marker icons --------
    {
        Scene scene{"MarkerIconSuppression"};

        Entity teamOnly = scene.CreateEntity("Team Only");
        teamOnly.AddComponent<TeamComponent>();
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(teamOnly) == SceneViewIconKind::None,
                     "team-only metadata does not draw a scene icon");

        Entity spawnWithMesh = scene.CreateEntity("Spawn With Mesh");
        spawnWithMesh.AddComponent<SpawnPointComponent>();
        MeshRendererComponent mesh;
        mesh.visible = true;
        spawnWithMesh.AddComponent<MeshRendererComponent>(mesh);
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(spawnWithMesh) == SceneViewIconKind::None,
                     "visible mesh suppresses marker icon");

        Entity hiddenMeshMarker = scene.CreateEntity("Hidden Mesh Marker");
        hiddenMeshMarker.AddComponent<SpawnPointComponent>();
        MeshRendererComponent hiddenMesh;
        hiddenMesh.visible = false;
        hiddenMeshMarker.AddComponent<MeshRendererComponent>(hiddenMesh);
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(hiddenMeshMarker) == SceneViewIconKind::Spawn,
                     "hidden mesh still allows marker icon");

        Entity cameraWithMesh = scene.CreateEntity("Camera With Mesh");
        cameraWithMesh.AddComponent<CameraComponent>();
        cameraWithMesh.AddComponent<MeshRendererComponent>(mesh);
        HK_CHECK_MSG(SceneViewOverlay::IconKindForEntity(cameraWithMesh) == SceneViewIconKind::Camera,
                     "camera icon remains available even if a mesh exists");
    }

    // --- billboard icon picking returns camera/light entities by screen pos -
    {
        Scene scene{"IconPicking"};
        Entity cameraEntity = scene.CreateEntity("Camera");
        cameraEntity.AddComponent<CameraComponent>();
        Entity lightEntity = scene.CreateEntity("Point Light");
        lightEntity.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, 0.0f};
        lightEntity.AddComponent<LightComponent>().type = LightComponent::Type::Point;
        Entity spawnEntity = scene.CreateEntity("Spawn");
        spawnEntity.GetComponent<TransformComponent>().localPosition = {-2.0f, 0.0f, 0.0f};
        spawnEntity.AddComponent<SpawnPointComponent>();

        const CameraRenderData camera = MakeOverlayCamera(800.0f / 600.0f);
        const UUID pickedCenter = SceneViewOverlay::PickIcon(scene, camera, {800.0f, 600.0f}, {400.0f, 300.0f});
        HK_CHECK_MSG(pickedCenter == cameraEntity.GetUUID(), "center icon pick selects camera");

        const SceneViewProjection lightProjection = SceneViewOverlay::ProjectWorldToViewportPixels(
            camera, {2.0f, 0.0f, 0.0f}, {800.0f, 600.0f});
        HK_CHECK_MSG(lightProjection.visible, "light projects into viewport");
        const UUID pickedLight = SceneViewOverlay::PickIcon(scene, camera, {800.0f, 600.0f}, lightProjection.pixels);
        HK_CHECK_MSG(pickedLight == lightEntity.GetUUID(), "light icon pick selects light");

        const SceneViewProjection spawnProjection = SceneViewOverlay::ProjectWorldToViewportPixels(
            camera, {-2.0f, 0.0f, 0.0f}, {800.0f, 600.0f});
        HK_CHECK_MSG(spawnProjection.visible, "spawn marker projects into viewport");
        const UUID pickedSpawn = SceneViewOverlay::PickIcon(scene, camera, {800.0f, 600.0f}, spawnProjection.pixels);
        HK_CHECK_MSG(pickedSpawn == spawnEntity.GetUUID(), "spawn icon pick selects spawn marker");

        const UUID missed = SceneViewOverlay::PickIcon(scene, camera, {800.0f, 600.0f}, {32.0f, 32.0f});
        HK_CHECK_MSG(!missed.IsValid(), "empty screen point misses icons");
    }

    // --- orientation gizmo axis mapping matches expected view directions ----
    {
        HK_CHECK_NEAR(glm::dot(SceneViewOverlay::ForwardForAxis(SceneViewAxis::PositiveX), glm::vec3(-1, 0, 0)), 1.0f,
                      1e-5);
        HK_CHECK_NEAR(glm::dot(SceneViewOverlay::ForwardForAxis(SceneViewAxis::NegativeX), glm::vec3(1, 0, 0)), 1.0f,
                      1e-5);
        HK_CHECK_NEAR(glm::dot(SceneViewOverlay::ForwardForAxis(SceneViewAxis::PositiveZ), glm::vec3(0, 0, -1)), 1.0f,
                      1e-5);
        HK_CHECK_MSG(glm::length(SceneViewOverlay::ForwardForAxis(SceneViewAxis::Isometric)) > 0.99f,
                     "isometric direction is normalized");
    }

    // --- EditorCamera can snap to an orientation around its current pivot ----
    {
        EditorCamera camera;
        camera.SetViewDirection(SceneViewOverlay::ForwardForAxis(SceneViewAxis::PositiveX));
        HK_CHECK_NEAR(glm::dot(camera.Forward(), glm::vec3(-1.0f, 0.0f, 0.0f)), 1.0f, 1e-4);

        camera.SetViewDirection(SceneViewOverlay::ForwardForAxis(SceneViewAxis::PositiveY));
        HK_CHECK_MSG(camera.Forward().y < -0.999f, "top view points down world Y");
    }
}
