#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"

using namespace Hockey;

namespace {

const FieldMetadata* FindField(const ComponentMetadata& md, const std::string& name) {
    for (const FieldMetadata& field : md.fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

} // namespace

void RunRenderComponentTests() {
    HockeyTest::BeginSuite("RenderComponentTests");

    // --- Add / query render components ---
    {
        Scene scene("RenderScene");
        Entity camera = scene.CreateEntity("Camera");
        CameraComponent cam;
        cam.fovDegrees = 60.0f;
        cam.primary = true;
        cam.followPlayer = true;
        cam.followOffset = {1.0f, 2.0f, 3.0f};
        cam.followRotation = {-20.0f, 5.0f, 0.0f};
        camera.AddComponent<CameraComponent>(cam);
        HK_CHECK(camera.HasComponent<CameraComponent>());
        HK_CHECK(camera.GetComponent<CameraComponent>().primary);
        HK_CHECK(camera.GetComponent<CameraComponent>().followPlayer);
        HK_CHECK_NEAR(camera.GetComponent<CameraComponent>().followOffset.y, 2.0f, 1e-5);
        HK_CHECK_NEAR(camera.GetComponent<CameraComponent>().followRotation.x, -20.0f, 1e-5);
        HK_CHECK_NEAR(camera.GetComponent<CameraComponent>().fovDegrees, 60.0f, 1e-5);

        Entity skinned = scene.CreateEntity("SkinnedPlayer");
        SkinnedMeshRendererComponent skinnedRenderer;
        skinnedRenderer.meshAsset = 101u;
        skinnedRenderer.materialAsset = 202u;
        skinnedRenderer.skeletonAsset = 303u;
        skinnedRenderer.visible = false;
        skinnedRenderer.castsShadows = true;
        skinnedRenderer.receivesShadows = false;
        skinned.AddComponent<SkinnedMeshRendererComponent>(skinnedRenderer);
        HK_CHECK(skinned.HasComponent<SkinnedMeshRendererComponent>());
        HK_CHECK_EQ(skinned.GetComponent<SkinnedMeshRendererComponent>().skeletonAsset, 303u);
        HK_CHECK(!skinned.GetComponent<SkinnedMeshRendererComponent>().visible);

        AnimationPosePaletteComponent palette;
        palette.skinningMatrices.push_back(glm::mat4{1.0f});
        palette.valid = true;
        skinned.AddComponent<AnimationPosePaletteComponent>(palette);
        HK_CHECK(skinned.HasComponent<AnimationPosePaletteComponent>());
        HK_CHECK_EQ(skinned.GetComponent<AnimationPosePaletteComponent>().skinningMatrices.size(), 1u);
        HK_CHECK(skinned.GetComponent<AnimationPosePaletteComponent>().valid);
    }

    // --- Enum string conversion round-trip ---
    HK_CHECK(LightTypeFromString(LightTypeToString(LightComponent::Type::Directional)) ==
             LightComponent::Type::Directional);
    HK_CHECK(LightTypeFromString(LightTypeToString(LightComponent::Type::Point)) == LightComponent::Type::Point);
    HK_CHECK(LightTypeFromString(LightTypeToString(LightComponent::Type::Spot)) == LightComponent::Type::Spot);
    HK_CHECK(LightTypeFromString("Garbage") == LightComponent::Type::Directional);

    // --- Serialization round-trip ---
    {
        Scene scene("RenderRoundTrip");

        Entity cameraEntity = scene.CreateEntity("MainCamera");
        CameraComponent cam;
        cam.fovDegrees = 55.0f;
        cam.nearClip = 0.25f;
        cam.farClip = 500.0f;
        cam.primary = true;
        cam.followPlayer = true;
        cam.followOffset = {3.0f, 9.0f, -11.0f};
        cam.followRotation = {-30.0f, 12.0f, 0.0f};
        cameraEntity.AddComponent<CameraComponent>(cam);

        Entity meshEntity = scene.CreateEntity("Rink");
        MeshRendererComponent mesh;
        mesh.meshAsset = 16045454102454522057ull;
        mesh.materialAsset = 18328928024750434439ull;
        mesh.visible = true;
        mesh.castsShadows = false;
        mesh.receivesShadows = true;
        meshEntity.AddComponent<MeshRendererComponent>(mesh);

        Entity skinnedEntity = scene.CreateEntity("AnimatedSkater");
        SkinnedMeshRendererComponent skinnedMesh;
        skinnedMesh.meshAsset = 7444ull;
        skinnedMesh.materialAsset = 8555ull;
        skinnedMesh.skeletonAsset = 9666ull;
        skinnedMesh.visible = false;
        skinnedMesh.castsShadows = true;
        skinnedMesh.receivesShadows = false;
        skinnedEntity.AddComponent<SkinnedMeshRendererComponent>(skinnedMesh);
        AnimationPosePaletteComponent palette;
        palette.skinningMatrices.push_back(glm::mat4{1.0f});
        palette.valid = true;
        skinnedEntity.AddComponent<AnimationPosePaletteComponent>(palette);

        Entity lightEntity = scene.CreateEntity("Sun");
        LightComponent light;
        light.type = LightComponent::Type::Spot;
        light.color = {0.9f, 0.8f, 0.7f};
        light.intensity = 3.5f;
        light.range = 42.0f;
        light.innerConeDegrees = 12.0f;
        light.outerConeDegrees = 28.0f;
        light.castsShadows = true;
        lightEntity.AddComponent<LightComponent>(light);
        EnvironmentComponent env;
        env.ambientColor = {0.1f, 0.12f, 0.15f};
        env.ambientIntensity = 0.8f;
        lightEntity.AddComponent<EnvironmentComponent>(env);

        Entity decalEntity = scene.CreateEntity("Logo");
        DecalComponent decal;
        decal.materialAsset = 12160806417561339961ull;
        decal.size = {2.0f, 1.0f, 2.0f};
        decal.affectsBaseColor = true;
        decal.affectsNormals = false;
        decalEntity.AddComponent<DecalComponent>(decal);
        ReflectionProbeComponent probe;
        probe.radius = 15.0f;
        probe.intensity = 0.5f;
        decalEntity.AddComponent<ReflectionProbeComponent>(probe);

        const std::filesystem::path path = Paths::TempFile("render_roundtrip.scene.yaml");
        SceneSerializer serializer(scene);
        HK_CHECK(static_cast<bool>(serializer.Serialize(path)));

        Scene loaded("Empty");
        SceneSerializer deserializer(loaded);
        HK_CHECK(static_cast<bool>(deserializer.Deserialize(path)));

        Entity loadedCamera = loaded.FindEntityByUUID(cameraEntity.GetUUID());
        HK_CHECK(loadedCamera.IsValid());
        HK_CHECK(loadedCamera.HasComponent<CameraComponent>());
        if (loadedCamera.HasComponent<CameraComponent>()) {
            const auto& c = loadedCamera.GetComponent<CameraComponent>();
            HK_CHECK_NEAR(c.fovDegrees, 55.0f, 1e-4);
            HK_CHECK_NEAR(c.nearClip, 0.25f, 1e-4);
            HK_CHECK_NEAR(c.farClip, 500.0f, 1e-3);
            HK_CHECK(c.primary);
            HK_CHECK(c.followPlayer);
            HK_CHECK_NEAR(c.followOffset.x, 3.0f, 1e-4);
            HK_CHECK_NEAR(c.followOffset.y, 9.0f, 1e-4);
            HK_CHECK_NEAR(c.followOffset.z, -11.0f, 1e-4);
            HK_CHECK_NEAR(c.followRotation.x, -30.0f, 1e-4);
            HK_CHECK_NEAR(c.followRotation.y, 12.0f, 1e-4);
        }

        Entity loadedMesh = loaded.FindEntityByUUID(meshEntity.GetUUID());
        HK_CHECK(loadedMesh.HasComponent<MeshRendererComponent>());
        if (loadedMesh.HasComponent<MeshRendererComponent>()) {
            const auto& m = loadedMesh.GetComponent<MeshRendererComponent>();
            HK_CHECK_EQ(m.meshAsset, 16045454102454522057ull);
            HK_CHECK_EQ(m.materialAsset, 18328928024750434439ull);
            HK_CHECK(m.visible);
            HK_CHECK(!m.castsShadows);
            HK_CHECK(m.receivesShadows);
        }

        Entity loadedSkinned = loaded.FindEntityByUUID(skinnedEntity.GetUUID());
        HK_CHECK(loadedSkinned.HasComponent<SkinnedMeshRendererComponent>());
        if (loadedSkinned.HasComponent<SkinnedMeshRendererComponent>()) {
            const auto& m = loadedSkinned.GetComponent<SkinnedMeshRendererComponent>();
            HK_CHECK_EQ(m.meshAsset, 7444ull);
            HK_CHECK_EQ(m.materialAsset, 8555ull);
            HK_CHECK_EQ(m.skeletonAsset, 9666ull);
            HK_CHECK(!m.visible);
            HK_CHECK(m.castsShadows);
            HK_CHECK(!m.receivesShadows);
        }
        HK_CHECK(!loadedSkinned.HasComponent<AnimationPosePaletteComponent>());

        Entity loadedLight = loaded.FindEntityByUUID(lightEntity.GetUUID());
        HK_CHECK(loadedLight.HasComponent<LightComponent>());
        HK_CHECK(loadedLight.HasComponent<EnvironmentComponent>());
        if (loadedLight.HasComponent<LightComponent>()) {
            const auto& l = loadedLight.GetComponent<LightComponent>();
            HK_CHECK(l.type == LightComponent::Type::Spot);
            HK_CHECK_NEAR(l.color.r, 0.9f, 1e-4);
            HK_CHECK_NEAR(l.intensity, 3.5f, 1e-4);
            HK_CHECK_NEAR(l.range, 42.0f, 1e-3);
            HK_CHECK_NEAR(l.outerConeDegrees, 28.0f, 1e-4);
        }

        Entity loadedDecal = loaded.FindEntityByUUID(decalEntity.GetUUID());
        HK_CHECK(loadedDecal.HasComponent<DecalComponent>());
        HK_CHECK(loadedDecal.HasComponent<ReflectionProbeComponent>());
        if (loadedDecal.HasComponent<DecalComponent>()) {
            const auto& d = loadedDecal.GetComponent<DecalComponent>();
            HK_CHECK_EQ(d.materialAsset, 12160806417561339961ull);
            HK_CHECK_NEAR(d.size.x, 2.0f, 1e-4);
            HK_CHECK(d.affectsBaseColor);
            HK_CHECK(!d.affectsNormals);
        }
    }

    // --- Metadata registration ---
    {
        ComponentRegistry& registry = ComponentRegistry::Get();
        registry.RegisterPhase2Components();
        const char* names[] = {"CameraComponent",      "MeshRendererComponent",    "SkinnedMeshRendererComponent",
                               "LightComponent",       "EnvironmentComponent",     "ReflectionProbeComponent",
                               "DecalComponent"};
        for (const char* name : names) {
            HK_CHECK_MSG(registry.FindByName(name) != nullptr, name);
        }
        const ComponentMetadata* lightMeta = registry.FindByName("LightComponent");
        if (lightMeta != nullptr) {
            bool foundEnum = false;
            for (const FieldMetadata& field : lightMeta->fields) {
                if (field.type == FieldType::Enum && field.enumNames.size() == 3) {
                    foundEnum = true;
                }
            }
            HK_CHECK(foundEnum);
        }
        const ComponentMetadata* cameraMeta = registry.FindByName("CameraComponent");
        if (cameraMeta != nullptr) {
            const FieldMetadata* followPlayer = FindField(*cameraMeta, "FollowPlayer");
            HK_CHECK(followPlayer != nullptr && followPlayer->type == FieldType::Bool);
            const FieldMetadata* followOffset = FindField(*cameraMeta, "FollowOffset");
            HK_CHECK(followOffset != nullptr && followOffset->type == FieldType::Vec3);
            HK_CHECK(followOffset != nullptr && followOffset->visibleWhenField == "FollowPlayer");
            const FieldMetadata* followRotation = FindField(*cameraMeta, "FollowRotation");
            HK_CHECK(followRotation != nullptr && followRotation->type == FieldType::Vec3);
            HK_CHECK(followRotation != nullptr && followRotation->visibleWhenField == "FollowPlayer");
        }
        const ComponentMetadata* skinnedMeta = registry.FindByName("SkinnedMeshRendererComponent");
        if (skinnedMeta != nullptr) {
            const FieldMetadata* meshAsset = FindField(*skinnedMeta, "MeshAsset");
            HK_CHECK(meshAsset != nullptr && meshAsset->type == FieldType::AssetRef);
            HK_CHECK(meshAsset != nullptr && meshAsset->assetTypeName == "Mesh");
            const FieldMetadata* materialAsset = FindField(*skinnedMeta, "MaterialAsset");
            HK_CHECK(materialAsset != nullptr && materialAsset->type == FieldType::AssetRef);
            HK_CHECK(materialAsset != nullptr && materialAsset->assetTypeName == "Material");
            const FieldMetadata* skeletonAsset = FindField(*skinnedMeta, "SkeletonAsset");
            HK_CHECK(skeletonAsset != nullptr && skeletonAsset->type == FieldType::AssetRef);
            HK_CHECK(skeletonAsset != nullptr && skeletonAsset->assetTypeName == "Skeleton");
        }
        HK_CHECK(registry.FindByName("AnimationPosePaletteComponent") == nullptr);
    }
}
