#include "Hockey/Editor/Project/EditorAssetPreviewRenderer.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

CameraRenderData MakeMaterialPreviewCamera() {
    TransformComponent transform;
    transform.localPosition = {0.0f, 0.15f, 1.9f};
    transform.localRotation = {-4.0f, 0.0f, 0.0f};

    CameraComponent camera;
    camera.fovDegrees = 32.0f;
    camera.nearClip = 0.05f;
    camera.farClip = 10.0f;
    return BuildCameraRenderData(transform, camera, 1.0f);
}

} // namespace

std::size_t EditorAssetPreviewRenderer::MaterialPreviewKeyHash::operator()(
    const MaterialPreviewKey& key) const noexcept {
    const std::uint64_t mixed = key.materialAssetId ^ (static_cast<std::uint64_t>(key.previewSize) << 32u);
    return std::hash<std::uint64_t>{}(mixed);
}

std::uint64_t EditorAssetPreviewRenderer::MaterialPreviewTextureId(EditorContext& context,
                                                                   std::uint64_t materialAssetId,
                                                                   std::uint32_t previewSize) {
    if (context.renderer == nullptr || context.imguiBridge == nullptr || !context.renderer->CanRender() ||
        context.assetManager == nullptr || materialAssetId == 0 || previewSize == 0) {
        return 0;
    }
    const AssetMetadata* sphereMesh =
        context.assetManager->Database().FindByRawPath("data/raw/meshes/sphere/sphere_mesh.mesh.yaml");
    if (sphereMesh == nullptr) {
        return 0;
    }

    TextureHandle& target = m_MaterialPreviewTargets[{materialAssetId, previewSize}];
    if (!target.IsValid()) {
        RenderTargetDesc desc;
        desc.width = previewSize;
        desc.height = previewSize;
        desc.debugName = "EditorMaterialPreview." + std::to_string(materialAssetId) + "." + std::to_string(previewSize);
        target = context.renderer->CreateRenderTarget(desc);
        if (!target.IsValid()) {
            return 0;
        }
    }

    Scene scene("EditorMaterialPreview");

    Entity environment = scene.CreateEntity("Environment");
    auto& env = environment.AddComponent<EnvironmentComponent>();
    env.ambientColor = {0.18f, 0.18f, 0.20f};
    env.ambientIntensity = 0.9f;

    Entity keyLight = scene.CreateEntity("KeyLight");
    keyLight.GetComponent<TransformComponent>().localRotation = {-38.0f, -28.0f, 0.0f};
    auto& light = keyLight.AddComponent<LightComponent>();
    light.type = LightComponent::Type::Directional;
    light.color = {1.0f, 0.96f, 0.88f};
    light.intensity = 3.5f;
    light.castsShadows = false;

    Entity sphere = scene.CreateEntity("MaterialPreviewSphere");
    auto& mesh = sphere.AddComponent<MeshRendererComponent>();
    mesh.meshAsset = sphereMesh->id.Value();
    mesh.materialAsset = materialAssetId;
    mesh.castsShadows = false;

    context.renderer->RenderSceneToTarget(scene, MakeMaterialPreviewCamera(), target);
    return context.imguiBridge->ViewportTextureId(target);
}

void EditorAssetPreviewRenderer::Clear() {
    m_MaterialPreviewTargets.clear();
}

} // namespace Hockey
