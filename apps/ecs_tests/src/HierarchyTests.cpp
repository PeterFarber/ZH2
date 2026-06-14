#include "Test.hpp"

#include <algorithm>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

namespace {

bool ChildrenContains(Scene& scene, Entity parent, Entity child) {
    for (Entity e : scene.GetChildren(parent)) {
        if (e == child) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunHierarchyTests() {
    HockeyTest::BeginSuite("HierarchyTests");

    Scene scene("Test");
    Entity root = scene.CreateEntity("Root");
    Entity child = scene.CreateEntity("Child");
    Entity grandChild = scene.CreateEntity("GrandChild");

    scene.SetParent(child, root);
    HK_CHECK(ChildrenContains(scene, root, child));
    HK_CHECK(child.HasComponent<ParentComponent>());
    HK_CHECK(child.GetComponent<ParentComponent>().parentId == root.GetUUID());

    scene.SetParent(grandChild, child);
    HK_CHECK(scene.IsDescendantOf(grandChild, root));
    HK_CHECK(!scene.IsDescendantOf(root, grandChild));

    // Cannot parent an entity to itself.
    scene.SetParent(root, root);
    HK_CHECK(!root.HasComponent<ParentComponent>());

    // Cannot parent an entity to its descendant.
    scene.SetParent(root, grandChild);
    HK_CHECK(!root.HasComponent<ParentComponent>());

    // RemoveParent detaches and clears link.
    scene.RemoveParent(child);
    HK_CHECK(!child.HasComponent<ParentComponent>());
    HK_CHECK(!ChildrenContains(scene, root, child));

    // Roots: root and child are now roots; grandChild still under child.
    const auto roots = scene.GetRootEntities();
    HK_CHECK(std::find(roots.begin(), roots.end(), root) != roots.end());
    HK_CHECK(std::find(roots.begin(), roots.end(), child) != roots.end());
    HK_CHECK(std::find(roots.begin(), roots.end(), grandChild) == roots.end());

    // DestroyEntity detaches children to root (preserving them).
    Entity p = scene.CreateEntity("P");
    Entity c1 = scene.CreateEntity("C1");
    Entity c2 = scene.CreateEntity("C2");
    scene.SetParent(c1, p);
    scene.SetParent(c2, p);
    const UUID c1Id = c1.GetUUID();
    const UUID c2Id = c2.GetUUID();
    scene.DestroyEntity(p);
    HK_CHECK(scene.ContainsUUID(c1Id));
    HK_CHECK(scene.ContainsUUID(c2Id));
    HK_CHECK(!scene.FindEntityByUUID(c1Id).HasComponent<ParentComponent>());

    // DestroyEntityRecursive removes the whole subtree.
    Entity rp = scene.CreateEntity("RP");
    Entity rc = scene.CreateEntity("RC");
    scene.SetParent(rc, rp);
    const UUID rpId = rp.GetUUID();
    const UUID rcId = rc.GetUUID();
    scene.DestroyEntityRecursive(rp);
    HK_CHECK(!scene.ContainsUUID(rpId));
    HK_CHECK(!scene.ContainsUUID(rcId));
}
