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

std::vector<UUID> EntityIds(const std::vector<Entity>& entities) {
    std::vector<UUID> ids;
    ids.reserve(entities.size());
    for (const Entity& entity : entities) {
        ids.push_back(entity.GetUUID());
    }
    return ids;
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

    // Root creation order is stable.
    {
        Scene ordered("OrderedRoots");
        Entity a = ordered.CreateEntity("A");
        Entity b = ordered.CreateEntity("B");
        Entity c = ordered.CreateEntity("C");

        const std::vector<UUID> roots = EntityIds(ordered.GetRootEntities());
        HK_CHECK_EQ(roots.size(), static_cast<std::size_t>(3));
        HK_CHECK(roots[0] == a.GetUUID());
        HK_CHECK(roots[1] == b.GetUUID());
        HK_CHECK(roots[2] == c.GetUUID());
    }

    // Same-parent reorder changes child order.
    {
        Scene ordered("OrderedChildren");
        Entity parent = ordered.CreateEntity("Parent");
        Entity a = ordered.CreateEntity("A");
        Entity b = ordered.CreateEntity("B");
        Entity c = ordered.CreateEntity("C");
        ordered.SetParent(a, parent, false);
        ordered.SetParent(b, parent, false);
        ordered.SetParent(c, parent, false);

        ordered.MoveEntity(c, parent, 0, false);

        const std::vector<UUID> children = EntityIds(ordered.GetChildren(parent));
        HK_CHECK_EQ(children.size(), static_cast<std::size_t>(3));
        HK_CHECK(children[0] == c.GetUUID());
        HK_CHECK(children[1] == a.GetUUID());
        HK_CHECK(children[2] == b.GetUUID());
        HK_CHECK_EQ(ordered.GetSiblingIndex(a), static_cast<std::size_t>(1));
    }

    // Reparent at index places the child at the requested sibling slot.
    {
        Scene ordered("ReparentAtIndex");
        Entity parentA = ordered.CreateEntity("ParentA");
        Entity parentB = ordered.CreateEntity("ParentB");
        Entity first = ordered.CreateEntity("First");
        Entity second = ordered.CreateEntity("Second");
        Entity moved = ordered.CreateEntity("Moved");
        ordered.SetParent(first, parentB, false);
        ordered.SetParent(second, parentB, false);
        ordered.SetParent(moved, parentA, false);

        ordered.MoveEntity(moved, parentB, 1, false);

        const std::vector<UUID> children = EntityIds(ordered.GetChildren(parentB));
        HK_CHECK_EQ(children.size(), static_cast<std::size_t>(3));
        HK_CHECK(children[0] == first.GetUUID());
        HK_CHECK(children[1] == moved.GetUUID());
        HK_CHECK(children[2] == second.GetUUID());
        HK_CHECK(ordered.GetParent(moved).GetUUID() == parentB.GetUUID());
    }

    // Move to root at index changes root order.
    {
        Scene ordered("RootAtIndex");
        Entity a = ordered.CreateEntity("A");
        Entity b = ordered.CreateEntity("B");
        Entity parent = ordered.CreateEntity("Parent");
        Entity child = ordered.CreateEntity("Child");
        ordered.SetParent(child, parent, false);

        ordered.MoveEntity(child, Entity{}, 1, false);

        const std::vector<UUID> roots = EntityIds(ordered.GetRootEntities());
        HK_CHECK_EQ(roots.size(), static_cast<std::size_t>(4));
        HK_CHECK(roots[0] == a.GetUUID());
        HK_CHECK(roots[1] == child.GetUUID());
        HK_CHECK(roots[2] == b.GetUUID());
        HK_CHECK(roots[3] == parent.GetUUID());
        HK_CHECK(!child.HasComponent<ParentComponent>());
    }

    // Invalid self/descendant moves leave the hierarchy untouched.
    {
        Scene ordered("InvalidMoves");
        Entity parent = ordered.CreateEntity("Parent");
        Entity child = ordered.CreateEntity("Child");
        Entity other = ordered.CreateEntity("Other");
        ordered.SetParent(child, parent, false);

        ordered.MoveEntity(parent, parent, 0, false);
        HK_CHECK(!parent.HasComponent<ParentComponent>());
        HK_CHECK(ordered.GetParent(child).GetUUID() == parent.GetUUID());

        ordered.MoveEntity(parent, child, 0, false);
        HK_CHECK(!parent.HasComponent<ParentComponent>());
        HK_CHECK(ordered.GetParent(child).GetUUID() == parent.GetUUID());

        const std::vector<UUID> roots = EntityIds(ordered.GetRootEntities());
        HK_CHECK_EQ(roots.size(), static_cast<std::size_t>(2));
        HK_CHECK(roots[0] == parent.GetUUID());
        HK_CHECK(roots[1] == other.GetUUID());
    }
}
