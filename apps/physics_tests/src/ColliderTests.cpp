#include "Test.hpp"

#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsShape.hpp"

using namespace Hockey;

void RunColliderTests() {
    HockeyTest::BeginSuite("ColliderTests");

    Physics::Init();

    // Box collider creates a shape.
    {
        BoxColliderComponent box;
        box.halfExtents = glm::vec3(0.5f, 1.0f, 0.25f);
        ShapeBuildResult result = TryBuildBoxShape(box);
        HK_CHECK_MSG(result.success, "box collider creates shape");
        HK_CHECK_MSG(result.volume > 0.0f, "box shape has volume");
    }

    // Sphere collider creates a shape.
    {
        SphereColliderComponent sphere;
        sphere.radius = 0.3f;
        ShapeBuildResult result = TryBuildSphereShape(sphere);
        HK_CHECK_MSG(result.success, "sphere collider creates shape");
    }

    // Capsule collider creates a shape.
    {
        CapsuleColliderComponent capsule;
        capsule.radius = 0.4f;
        capsule.halfHeight = 0.8f;
        ShapeBuildResult result = TryBuildCapsuleShape(capsule);
        HK_CHECK_MSG(result.success, "capsule collider creates shape");
    }

    // Cylinder collider creates a shape.
    {
        CylinderColliderComponent cylinder;
        cylinder.radius = 0.4f;
        cylinder.halfHeight = 0.5f;
        ShapeBuildResult result = TryBuildCylinderShape(cylinder);
        HK_CHECK_MSG(result.success, "cylinder collider creates shape");
    }

    // Invalid collider size validation fails.
    {
        BoxColliderComponent badBox;
        badBox.halfExtents = glm::vec3(0.0f, 1.0f, 1.0f);
        HK_CHECK_MSG(!IsValidBoxCollider(badBox), "zero-extent box invalid");
        HK_CHECK_MSG(!TryBuildBoxShape(badBox).success, "zero-extent box fails to build");

        SphereColliderComponent badSphere;
        badSphere.radius = -1.0f;
        HK_CHECK_MSG(!IsValidSphereCollider(badSphere), "negative-radius sphere invalid");

        CapsuleColliderComponent badCapsule;
        badCapsule.radius = 0.0f;
        HK_CHECK_MSG(!IsValidCapsuleCollider(badCapsule), "zero-radius capsule invalid");

        CylinderColliderComponent badCylinder;
        badCylinder.halfHeight = 0.0f;
        HK_CHECK_MSG(!IsValidCylinderCollider(badCylinder), "zero-height cylinder invalid");
    }

    // Mesh collider path handles missing asset gracefully (no crash, reports failure).
    {
        MeshColliderComponent mesh;
        mesh.meshAsset = 0;
        ShapeBuildResult result = TryBuildMeshShape(mesh);
        HK_CHECK_MSG(!result.success, "mesh collider with no asset fails gracefully");
        HK_CHECK_MSG(!result.error.empty(), "mesh collider failure has a reason");

        MeshColliderComponent meshWithAsset;
        meshWithAsset.meshAsset = 1234;
        ShapeBuildResult result2 = TryBuildMeshShape(meshWithAsset);
        HK_CHECK_MSG(!result2.success, "mesh collider without resolvable data fails gracefully");
    }
}
