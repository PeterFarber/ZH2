#include "Hockey/Physics/Jolt/JoltShapeFactory.hpp"

#include <string>
#include <vector>

#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"
#include "Hockey/Physics/PhysicsMeshProvider.hpp"

namespace Hockey::JoltDetail {

namespace {

// Jolt's default convex radius; box/cylinder half extents must exceed it.
constexpr float kConvexRadius = 0.05f;

JPH::Ref<JPH::Shape> Finalize(const JPH::ShapeSettings::ShapeResult& result, std::string& outError) {
    if (result.HasError()) {
        outError = std::string(result.GetError().c_str());
        return {};
    }
    return result.Get();
}

bool IsIdentityOffset(const glm::vec3& offset, const glm::vec3& eulerDegrees) {
    return offset == glm::vec3(0.0f) && eulerDegrees == glm::vec3(0.0f);
}

} // namespace

JPH::Ref<JPH::Shape> CreateBoxShape(const BoxColliderComponent& c, std::string& outError) {
    if (c.halfExtents.x <= 0.0f || c.halfExtents.y <= 0.0f || c.halfExtents.z <= 0.0f) {
        outError = "box collider has non-positive half extents";
        return {};
    }
    const float convexRadius =
        JPH::min(kConvexRadius, JPH::min(c.halfExtents.x, JPH::min(c.halfExtents.y, c.halfExtents.z)) * 0.5f);
    JPH::BoxShapeSettings settings(ToJolt(c.halfExtents), convexRadius);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> CreateSphereShape(const SphereColliderComponent& c, std::string& outError) {
    if (c.radius <= 0.0f) {
        outError = "sphere collider has non-positive radius";
        return {};
    }
    JPH::SphereShapeSettings settings(c.radius);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> CreateCapsuleShape(const CapsuleColliderComponent& c, std::string& outError) {
    if (c.radius <= 0.0f || c.halfHeight <= 0.0f) {
        outError = "capsule collider has non-positive radius/half height";
        return {};
    }
    JPH::CapsuleShapeSettings settings(c.halfHeight, c.radius);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> CreateCylinderShape(const CylinderColliderComponent& c, std::string& outError) {
    if (c.radius <= 0.0f || c.halfHeight <= 0.0f) {
        outError = "cylinder collider has non-positive radius/half height";
        return {};
    }
    const float convexRadius = JPH::min(kConvexRadius, JPH::min(c.radius, c.halfHeight) * 0.5f);
    JPH::CylinderShapeSettings settings(c.halfHeight, c.radius, convexRadius);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> CreateMeshShape(const MeshColliderComponent& c, std::string& outError) {
    if (c.meshAsset == 0) {
        outError = "mesh collider has no mesh asset assigned";
        return {};
    }

    // hockey_physics does not depend on hockey_assets, so the app installs a
    // provider (PhysicsMeshRegistry) that resolves the mesh id into geometry.
    const PhysicsMeshRegistry& registry = PhysicsMeshRegistry::Get();
    PhysicsMeshData data;
    if (!registry.Resolve(c.meshAsset, data)) {
        outError = registry.HasProvider()
                       ? "mesh collider geometry unavailable for asset " + std::to_string(c.meshAsset)
                       : "no mesh-data provider installed for mesh colliders";
        return {};
    }
    if (data.vertices.empty()) {
        outError = "mesh collider resolved to an empty vertex set";
        return {};
    }

    if (c.convex) {
        // A convex hull works for dynamic bodies and is cheap to evaluate.
        JPH::Array<JPH::Vec3> points;
        points.reserve(data.vertices.size());
        for (const glm::vec3& v : data.vertices) {
            points.push_back(ToJolt(v));
        }
        JPH::ConvexHullShapeSettings settings(points);
        return Finalize(settings.Create(), outError);
    }

    // Concave triangle mesh. Jolt restricts MeshShape to static/kinematic
    // bodies; the scene validator already flags dynamic non-convex colliders.
    if (data.indices.size() < 3) {
        outError = "mesh collider resolved to no triangles";
        return {};
    }
    JPH::VertexList vertices;
    vertices.reserve(static_cast<JPH::uint>(data.vertices.size()));
    for (const glm::vec3& v : data.vertices) {
        vertices.push_back(JPH::Float3(v.x, v.y, v.z));
    }
    JPH::IndexedTriangleList triangles;
    triangles.reserve(static_cast<JPH::uint>(data.indices.size() / 3));
    for (std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
        triangles.push_back(JPH::IndexedTriangle(data.indices[i], data.indices[i + 1], data.indices[i + 2], 0));
    }
    JPH::MeshShapeSettings settings(vertices, triangles);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> ApplyOffset(const JPH::Ref<JPH::Shape>& shape, const glm::vec3& offset,
                                 const glm::vec3& eulerDegrees, std::string& outError) {
    if (shape == nullptr) {
        return {};
    }
    if (IsIdentityOffset(offset, eulerDegrees)) {
        return shape;
    }
    JPH::RotatedTranslatedShapeSettings settings(ToJolt(offset), EulerDegreesToJolt(eulerDegrees), shape);
    return Finalize(settings.Create(), outError);
}

JPH::Ref<JPH::Shape> CreateEntityShape(const Entity& entity, std::string& outError, bool& outAllTriggers,
                                       bool& outAnyTrigger) {
    struct SubShape {
        JPH::Ref<JPH::Shape> shape;
        bool isTrigger = false;
    };
    std::vector<SubShape> subShapes;

    auto tryAdd = [&](JPH::Ref<JPH::Shape> shape, const glm::vec3& offset, const glm::vec3& rotation, bool isTrigger) {
        if (shape == nullptr) {
            return;
        }
        std::string offsetError;
        JPH::Ref<JPH::Shape> placed = ApplyOffset(shape, offset, rotation, offsetError);
        if (placed != nullptr) {
            subShapes.push_back({placed, isTrigger});
        }
    };

    std::string err;
    if (entity.HasComponent<BoxColliderComponent>()) {
        const auto& c = entity.GetComponent<BoxColliderComponent>();
        tryAdd(CreateBoxShape(c, err), c.offset, c.rotation, c.isTrigger);
    }
    if (entity.HasComponent<SphereColliderComponent>()) {
        const auto& c = entity.GetComponent<SphereColliderComponent>();
        tryAdd(CreateSphereShape(c, err), c.offset, glm::vec3(0.0f), c.isTrigger);
    }
    if (entity.HasComponent<CapsuleColliderComponent>()) {
        const auto& c = entity.GetComponent<CapsuleColliderComponent>();
        tryAdd(CreateCapsuleShape(c, err), c.offset, c.rotation, c.isTrigger);
    }
    if (entity.HasComponent<CylinderColliderComponent>()) {
        const auto& c = entity.GetComponent<CylinderColliderComponent>();
        tryAdd(CreateCylinderShape(c, err), c.offset, c.rotation, c.isTrigger);
    }
    if (entity.HasComponent<MeshColliderComponent>()) {
        const auto& c = entity.GetComponent<MeshColliderComponent>();
        // Mesh geometry is local to the asset, so it carries no offset/rotation.
        tryAdd(CreateMeshShape(c, err), glm::vec3(0.0f), glm::vec3(0.0f), c.isTrigger);
    }

    if (subShapes.empty()) {
        outError = err.empty() ? "entity has no valid collider" : err;
        return {};
    }

    outAllTriggers = true;
    outAnyTrigger = false;
    for (const SubShape& s : subShapes) {
        outAllTriggers = outAllTriggers && s.isTrigger;
        outAnyTrigger = outAnyTrigger || s.isTrigger;
    }

    if (subShapes.size() == 1) {
        return subShapes.front().shape;
    }

    JPH::StaticCompoundShapeSettings compound;
    for (const SubShape& s : subShapes) {
        compound.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), s.shape);
    }
    return Finalize(compound.Create(), outError);
}

} // namespace Hockey::JoltDetail
