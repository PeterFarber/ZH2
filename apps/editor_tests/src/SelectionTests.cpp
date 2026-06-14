#include "Test.hpp"

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Selection.hpp"

using namespace Hockey;

void RunSelectionTests() {
    HockeyTest::BeginSuite("SelectionTests");

    const UUID a(1001);
    const UUID b(2002);
    const UUID c(3003);

    // --- select entity ------------------------------------------------------
    {
        Selection selection;
        selection.Select(a);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.IsSelected(a), "selected entity reported selected");
        HK_CHECK_MSG(selection.Primary() == a, "single select sets primary");
    }

    // --- add entity ---------------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));
        HK_CHECK_MSG(selection.IsSelected(a) && selection.IsSelected(b), "both entities selected");
        HK_CHECK_MSG(selection.Primary() == b, "add makes the new entity primary");
        // adding a duplicate does not grow the selection
        selection.Add(a);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));
    }

    // --- remove entity ------------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Remove(b);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(!selection.IsSelected(b), "removed entity not selected");
        HK_CHECK_MSG(selection.Primary() == a, "removing primary repairs primary");
    }

    // --- toggle entity ------------------------------------------------------
    {
        Selection selection;
        selection.Toggle(a);
        HK_CHECK_MSG(selection.IsSelected(a), "toggle selects when absent");
        selection.Toggle(a);
        HK_CHECK_MSG(!selection.IsSelected(a), "toggle deselects when present");
        HK_CHECK_MSG(selection.Empty(), "selection empty after toggling off");
    }

    // --- clear selection ----------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Clear();
        HK_CHECK_MSG(selection.Empty(), "clear empties selection");
        HK_CHECK_MSG(!selection.Primary().IsValid(), "clear invalidates primary");
    }

    // --- primary selection / single-select replaces --------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Select(c); // single-select replaces the whole selection
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.Primary() == c, "select replaces and sets primary");
        // invalid id clears
        selection.Select(UUID(0));
        HK_CHECK_MSG(selection.Empty(), "selecting an invalid id clears selection");
    }

    // --- invalid selection removed after scene deletion ---------------------
    {
        Scene scene("SelectionTestScene");
        Entity e1 = scene.CreateEntity("E1");
        Entity e2 = scene.CreateEntity("E2");
        const UUID id1 = e1.GetUUID();
        const UUID id2 = e2.GetUUID();

        Selection selection;
        selection.Add(id1);
        selection.Add(id2);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));

        scene.DestroyEntity(e2);
        selection.Validate(scene);

        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.IsSelected(id1), "valid entity stays selected");
        HK_CHECK_MSG(!selection.IsSelected(id2), "deleted entity dropped from selection");
        HK_CHECK_MSG(selection.Primary() == id1, "primary repaired to a valid entity");
    }
}
