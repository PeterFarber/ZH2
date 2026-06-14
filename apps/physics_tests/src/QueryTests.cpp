#include "Test.hpp"

#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsLayer.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

Entity MakeStaticBox(Scene& scene, const char* name, const glm::vec3& pos, const glm::vec3& halfExtents,
                     PhysicsLayer layer, bool trigger) {
    Entity e = scene.CreateEntity(name);
    e.GetComponent<TransformComponent>().localPosition = pos;
    RigidBodyComponent rb;
    rb.type = RigidBodyType::Static;
    rb.layer = layer;
    e.AddComponent<RigidBodyComponent>(rb);
    BoxColliderComponent box;
    box.halfExtents = halfExtents;
    box.isTrigger = trigger;
    e.AddComponent<BoxColliderComponent>(box);
    return e;
}

bool ContainsEntity(const std::vector<OverlapHit>& hits, UUID id) {
    for (const OverlapHit& h : hits) {
        if (h.entity == id) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunQueryTests() {
    HockeyTest::BeginSuite("QueryTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    PhysicsWorld world;
    world.Init(MakeDefaultPhysicsSettings());

    Scene scene("Queries");
    Entity box = MakeStaticBox(scene, "Box", glm::vec3(0.0f), glm::vec3(1.0f), PhysicsLayer::Static, false);
    Entity goalTrigger =
        MakeStaticBox(scene, "GoalTrigger", glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f), PhysicsLayer::Goal, true);
    world.CreateBodiesFromScene(scene);

    const UUID boxId = box.GetUUID();
    const UUID triggerId = goalTrigger.GetUUID();

    // Raycast hits box.
    {
        RaycastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.maxDistance = 10.0f;
        RaycastHit hit;
        const bool ok = world.Raycast(req, hit);
        HK_CHECK_MSG(ok, "raycast hits box");
        HK_CHECK_MSG(hit.entity == boxId, "raycast reports the box entity");
        HK_CHECK_NEAR(hit.point.y, 1.0f, 0.1);
        HK_CHECK_MSG(hit.normal.y > 0.5f, "raycast surface normal points up");
    }

    // Raycast misses when direction wrong.
    {
        RaycastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        req.maxDistance = 10.0f;
        RaycastHit hit;
        HK_CHECK_MSG(!world.Raycast(req, hit), "raycast misses when pointing away");
    }

    // Raycast respects layer mask.
    {
        RaycastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.maxDistance = 10.0f;
        req.layerMask = ~(1u << static_cast<std::uint32_t>(PhysicsLayer::Static));
        RaycastHit hit;
        HK_CHECK_MSG(!world.Raycast(req, hit), "raycast respects layer mask");
    }

    // Overlap sphere detects body.
    {
        OverlapSphereRequest req;
        req.center = glm::vec3(0.0f);
        req.radius = 1.5f;
        std::vector<OverlapHit> hits;
        const bool ok = world.OverlapSphere(req, hits);
        HK_CHECK_MSG(ok, "overlap sphere detects body");
        HK_CHECK_MSG(ContainsEntity(hits, boxId), "overlap sphere reports the box");
    }

    // Overlap box detects body.
    {
        OverlapBoxRequest req;
        req.center = glm::vec3(0.0f);
        req.halfExtents = glm::vec3(1.5f);
        std::vector<OverlapHit> hits;
        const bool ok = world.OverlapBox(req, hits);
        HK_CHECK_MSG(ok, "overlap box detects body");
        HK_CHECK_MSG(ContainsEntity(hits, boxId), "overlap box reports the box");
    }

    // Trigger inclusion flag works.
    {
        OverlapSphereRequest excludeReq;
        excludeReq.center = glm::vec3(10.0f, 0.0f, 0.0f);
        excludeReq.radius = 1.5f;
        excludeReq.includeTriggers = false;
        std::vector<OverlapHit> excludeHits;
        world.OverlapSphere(excludeReq, excludeHits);
        HK_CHECK_MSG(!ContainsEntity(excludeHits, triggerId), "overlap excludes triggers when flag off");

        OverlapSphereRequest includeReq = excludeReq;
        includeReq.includeTriggers = true;
        std::vector<OverlapHit> includeHits;
        world.OverlapSphere(includeReq, includeHits);
        HK_CHECK_MSG(ContainsEntity(includeHits, triggerId), "overlap includes triggers when flag on");
    }

    world.Shutdown();
}
