#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Phase 3 render components. These hold only backend-agnostic data; the
// renderer resolves resource references to GPU handles.
// The ECS library never depends on the renderer.
//
// Mesh/material references are content-pipeline AssetIDs stored as raw uint64
// values because AssetID lives in hockey_assets, which ECS must not depend on.
// ---------------------------------------------------------------------------

struct CameraComponent {
    float fovDegrees = 70.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    bool primary = false;
    bool followPlayer = false;
    glm::vec3 followOffset{0.0f, 7.5f, 10.0f};
    glm::vec3 followRotation{-30.0f, 0.0f, 0.0f};
};

struct MeshRendererComponent {
    // 0 means no asset assigned.
    uint64_t meshAsset = 0;
    uint64_t materialAsset = 0;

    bool visible = true;
    bool castsShadows = true;
    bool receivesShadows = true;
};

struct SkinnedMeshRendererComponent {
    // 0 means no asset assigned. AssetID lives in hockey_assets, so ECS stores
    // the raw value to preserve the dependency boundary.
    uint64_t meshAsset = 0;
    uint64_t materialAsset = 0;
    uint64_t skeletonAsset = 0;

    bool visible = true;
    bool castsShadows = true;
    bool receivesShadows = true;
};

struct AnimationPosePaletteComponent {
    std::vector<glm::mat4> skinningMatrices;
    bool valid = false;
};

struct LightComponent {
    enum class Type { Directional, Point, Spot };

    Type type = Type::Directional;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 15.0f;
    float outerConeDegrees = 30.0f;
    bool castsShadows = true;
};

struct EnvironmentComponent {
    glm::vec3 ambientColor{0.03f};
    float ambientIntensity = 1.0f;
};

struct ReflectionProbeComponent {
    float radius = 10.0f;
    float intensity = 1.0f;
};

struct DecalComponent {
    uint64_t materialAsset = 0;
    glm::vec3 size{1.0f};
    bool affectsBaseColor = true;
    bool affectsNormals = true;
};

// LightComponent::Type string conversion for serialization/metadata.
std::string LightTypeToString(LightComponent::Type type);
LightComponent::Type LightTypeFromString(const std::string& value);

// Canonical names so events/serialization/metadata stay aligned.
template <> struct ComponentTraits<CameraComponent> {
    static constexpr const char* Name = "CameraComponent";
};
template <> struct ComponentTraits<MeshRendererComponent> {
    static constexpr const char* Name = "MeshRendererComponent";
};
template <> struct ComponentTraits<SkinnedMeshRendererComponent> {
    static constexpr const char* Name = "SkinnedMeshRendererComponent";
};
template <> struct ComponentTraits<AnimationPosePaletteComponent> {
    static constexpr const char* Name = "AnimationPosePaletteComponent";
};
template <> struct ComponentTraits<LightComponent> {
    static constexpr const char* Name = "LightComponent";
};
template <> struct ComponentTraits<EnvironmentComponent> {
    static constexpr const char* Name = "EnvironmentComponent";
};
template <> struct ComponentTraits<ReflectionProbeComponent> {
    static constexpr const char* Name = "ReflectionProbeComponent";
};
template <> struct ComponentTraits<DecalComponent> {
    static constexpr const char* Name = "DecalComponent";
};

} // namespace Hockey
