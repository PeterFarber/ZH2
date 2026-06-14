#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Phase 3 render components. These hold only backend-agnostic data; the
// renderer resolves resource references (mesh/material names) to GPU handles.
// The ECS library never depends on the renderer.
//
// Resource references prefer a content-pipeline AssetID, falling back to a
// built-in name when the asset id is 0/invalid, e.g.
//   meshName = "BuiltIn.RinkPlane", materialName = "BuiltIn.Ice".
// Asset ids are stored as raw uint64 values (the AssetID type lives in
// hockey_assets, which the ECS must not depend on); the renderer interprets
// them as AssetIDs.
// ---------------------------------------------------------------------------

struct CameraComponent {
    float fovDegrees = 70.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    bool primary = false;
};

struct MeshRendererComponent {
    // Preferred references (0 == none; fall back to the built-in names below).
    uint64_t meshAsset = 0;
    uint64_t materialAsset = 0;

    // Built-in fallback resource names, e.g. "BuiltIn.RinkPlane".
    std::string meshName;
    std::string materialName;
    bool visible = true;
    bool castsShadows = true;
    bool receivesShadows = true;
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
    std::string materialName;
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
